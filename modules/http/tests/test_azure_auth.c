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

#include <criterion/criterion.h>
#include "libtest/mock-transport.h"
#include "libtest/testutils.h"

#include "azure-auth.h"

Test(azure_auth, test_azure_auth_new_and_free)
{
  AzureAuth *auth = azure_auth_new();
  cr_assert_not_null(auth);
  
  azure_auth_free(auth);
}

Test(azure_auth, test_azure_auth_configuration)
{
  AzureAuth *auth = azure_auth_new();
  
  cr_assert_false(azure_auth_is_configured(auth));
  
  azure_auth_set_tenant_id(auth, "test-tenant-id");
  azure_auth_set_client_id(auth, "test-client-id");
  azure_auth_set_client_secret(auth, "test-client-secret");
  
  cr_assert_true(azure_auth_is_configured(auth));
  
  azure_auth_free(auth);
}

Test(azure_auth, test_azure_auth_scope_and_timeout)
{
  AzureAuth *auth = azure_auth_new();
  
  azure_auth_set_scope(auth, "https://custom.scope/.default");
  azure_auth_set_timeout(auth, 60);
  
  azure_auth_free(auth);
}

Test(azure_auth, test_azure_auth_token_without_config)
{
  AzureAuth *auth = azure_auth_new();
  GError *error = NULL;
  
  gchar *token = azure_auth_get_token(auth, &error);
  
  cr_assert_null(token);
  cr_assert_not_null(error);
  cr_assert_eq(error->domain, G_IO_ERROR);
  cr_assert_eq(error->code, G_IO_ERROR_NOT_INITIALIZED);
  
  g_clear_error(&error);
  azure_auth_free(auth);
}

Test(azure_auth, test_azure_auth_invalidate_token)
{
  AzureAuth *auth = azure_auth_new();
  
  // This should not crash even with no token
  azure_auth_invalidate_token(auth);
  
  azure_auth_free(auth);
}