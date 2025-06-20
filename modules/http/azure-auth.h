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

#ifndef AZURE_AUTH_H_INCLUDED
#define AZURE_AUTH_H_INCLUDED 1

#include "syslog-ng-config.h"
#include "compat/glib.h"
#include <curl/curl.h>
#include <time.h>

#define AZURE_AUTH_TOKEN_REFRESH_MARGIN 300  /* 5 minutes before expiry */
#define AZURE_AUTH_DEFAULT_TIMEOUT 30
#define AZURE_AUTH_LOGIN_URL "https://login.microsoftonline.com/%s/oauth2/v2.0/token"

typedef struct _AzureAuth AzureAuth;

struct _AzureAuth
{
  gchar *tenant_id;
  gchar *client_id;
  gchar *client_secret;
  gchar *scope;
  
  /* Token cache */
  gchar *access_token;
  time_t token_expires_at;
  GMutex token_mutex;
  
  /* HTTP client for auth requests */
  CURL *curl;
  glong timeout;
};

AzureAuth *azure_auth_new(void);
void azure_auth_free(AzureAuth *self);

void azure_auth_set_tenant_id(AzureAuth *self, const gchar *tenant_id);
void azure_auth_set_client_id(AzureAuth *self, const gchar *client_id);
void azure_auth_set_client_secret(AzureAuth *self, const gchar *client_secret);
void azure_auth_set_scope(AzureAuth *self, const gchar *scope);
void azure_auth_set_timeout(AzureAuth *self, glong timeout);

gboolean azure_auth_is_configured(AzureAuth *self);
gchar *azure_auth_get_token(AzureAuth *self, GError **error);
void azure_auth_invalidate_token(AzureAuth *self);

#endif