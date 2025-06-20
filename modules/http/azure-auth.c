/*
 * Copyright (c) 2024 One Identity LLC.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "azure-auth.h"
#include "messages.h"
#include "compat/glib.h"
#include <json-c/json.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
  GString *response;
} AzureAuthResponse;

static size_t
_azure_auth_write_callback(void *contents, size_t size, size_t nmemb, AzureAuthResponse *userp)
{
  size_t realsize = size * nmemb;
  g_string_append_len(userp->response, (const gchar *)contents, realsize);
  return realsize;
}

static gboolean
_parse_token_response(const gchar *response, gchar **access_token, glong *expires_in, GError **error)
{
  json_object *root = json_tokener_parse(response);
  if (!root)
    {
      g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, 
                  "Failed to parse JSON response from Azure AD");
      return FALSE;
    }

  json_object *token_obj;
  if (!json_object_object_get_ex(root, "access_token", &token_obj))
    {
      json_object *error_obj;
      if (json_object_object_get_ex(root, "error_description", &error_obj))
        {
          g_set_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                      "Azure AD authentication failed: %s", 
                      json_object_get_string(error_obj));
        }
      else
        {
          g_set_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                      "Azure AD authentication failed: no access_token in response");
        }
      json_object_put(root);
      return FALSE;
    }

  *access_token = g_strdup(json_object_get_string(token_obj));

  json_object *expires_obj;
  if (json_object_object_get_ex(root, "expires_in", &expires_obj))
    {
      *expires_in = json_object_get_int64(expires_obj);
    }
  else
    {
      *expires_in = 3600; /* Default to 1 hour if not specified */
    }

  json_object_put(root);
  return TRUE;
}

static gboolean
_azure_auth_fetch_token(AzureAuth *self, GError **error)
{
  AzureAuthResponse response = { .response = g_string_new("") };
  CURLcode res;
  gchar *auth_url;
  gchar *post_data;
  gchar *escaped_client_id, *escaped_client_secret, *escaped_scope;
  
  auth_url = g_strdup_printf(AZURE_AUTH_LOGIN_URL, self->tenant_id);
  
  /* URL encode the parameters */
  escaped_client_id = curl_easy_escape(self->curl, self->client_id, 0);
  escaped_client_secret = curl_easy_escape(self->curl, self->client_secret, 0);
  escaped_scope = curl_easy_escape(self->curl, self->scope ? self->scope : "https://monitor.azure.com/.default", 0);
  
  post_data = g_strdup_printf(
    "grant_type=client_credentials"
    "&client_id=%s"
    "&client_secret=%s"
    "&scope=%s",
    escaped_client_id, escaped_client_secret, escaped_scope);
  
  curl_free(escaped_client_id);
  curl_free(escaped_client_secret);
  curl_free(escaped_scope);

  curl_easy_setopt(self->curl, CURLOPT_URL, auth_url);
  curl_easy_setopt(self->curl, CURLOPT_POSTFIELDS, post_data);
  curl_easy_setopt(self->curl, CURLOPT_WRITEFUNCTION, _azure_auth_write_callback);
  curl_easy_setopt(self->curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(self->curl, CURLOPT_TIMEOUT, self->timeout);
  curl_easy_setopt(self->curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(self->curl, CURLOPT_SSL_VERIFYHOST, 2L);

  /* Set Content-Type header */
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
  curl_easy_setopt(self->curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(self->curl);
  curl_slist_free_all(headers);
  
  g_free(auth_url);
  g_free(post_data);

  if (res != CURLE_OK)
    {
      g_set_error(error, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
                  "Failed to authenticate with Azure AD: %s", curl_easy_strerror(res));
      g_string_free(response.response, TRUE);
      return FALSE;
    }

  glong http_code;
  curl_easy_getinfo(self->curl, CURLINFO_RESPONSE_CODE, &http_code);
  
  if (http_code != 200)
    {
      g_set_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                  "Azure AD authentication failed with HTTP %ld: %s", 
                  http_code, response.response->str);
      g_string_free(response.response, TRUE);
      return FALSE;
    }

  gchar *access_token;
  glong expires_in;
  
  if (!_parse_token_response(response.response->str, &access_token, &expires_in, error))
    {
      g_string_free(response.response, TRUE);
      return FALSE;
    }

  g_string_free(response.response, TRUE);

  /* Update cached token */
  g_free(self->access_token);
  self->access_token = access_token;
  self->token_expires_at = time(NULL) + expires_in - AZURE_AUTH_TOKEN_REFRESH_MARGIN;

  msg_debug("Azure authentication successful, token expires in seconds",
            evt_tag_long("expires_in", expires_in));

  return TRUE;
}

AzureAuth *
azure_auth_new(void)
{
  AzureAuth *self = g_new0(AzureAuth, 1);
  
  g_mutex_init(&self->token_mutex);
  self->curl = curl_easy_init();
  self->timeout = AZURE_AUTH_DEFAULT_TIMEOUT;
  
  return self;
}

void
azure_auth_free(AzureAuth *self)
{
  if (!self)
    return;
    
  g_free(self->tenant_id);
  g_free(self->client_id);
  g_free(self->client_secret);
  g_free(self->scope);
  g_free(self->access_token);
  
  if (self->curl)
    curl_easy_cleanup(self->curl);
    
  g_mutex_clear(&self->token_mutex);
  g_free(self);
}

void
azure_auth_set_tenant_id(AzureAuth *self, const gchar *tenant_id)
{
  g_free(self->tenant_id);
  self->tenant_id = g_strdup(tenant_id);
}

void
azure_auth_set_client_id(AzureAuth *self, const gchar *client_id)
{
  g_free(self->client_id);
  self->client_id = g_strdup(client_id);
}

void
azure_auth_set_client_secret(AzureAuth *self, const gchar *client_secret)
{
  g_free(self->client_secret);
  self->client_secret = g_strdup(client_secret);
}

void
azure_auth_set_scope(AzureAuth *self, const gchar *scope)
{
  g_free(self->scope);
  self->scope = g_strdup(scope);
}

void
azure_auth_set_timeout(AzureAuth *self, glong timeout)
{
  self->timeout = timeout;
}

gboolean
azure_auth_is_configured(AzureAuth *self)
{
  return (self->tenant_id && self->client_id && self->client_secret);
}

gchar *
azure_auth_get_token(AzureAuth *self, GError **error)
{
  if (!azure_auth_is_configured(self))
    {
      g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                  "Azure authentication not configured (missing tenant_id, client_id, or client_secret)");
      return NULL;
    }

  g_mutex_lock(&self->token_mutex);
  
  time_t now = time(NULL);
  
  /* Check if we need to refresh the token */
  if (!self->access_token || now >= self->token_expires_at)
    {
      if (self->access_token)
        msg_debug("Azure access token expired, refreshing");
      else
        msg_debug("No Azure access token cached, fetching new one");
        
      if (!_azure_auth_fetch_token(self, error))
        {
          g_mutex_unlock(&self->token_mutex);
          return NULL;
        }
    }
  
  gchar *token = g_strdup(self->access_token);
  g_mutex_unlock(&self->token_mutex);
  
  return token;
}

void
azure_auth_invalidate_token(AzureAuth *self)
{
  g_mutex_lock(&self->token_mutex);
  g_free(self->access_token);
  self->access_token = NULL;
  self->token_expires_at = 0;
  g_mutex_unlock(&self->token_mutex);
}