# Azure DCR Authentication for syslog-ng HTTP Destination

This document describes the Azure Data Collection Rule (DCR) authentication support added to the syslog-ng HTTP destination module.

## Overview

The Azure DCR authentication feature enables syslog-ng to authenticate with Azure Monitor's Data Collection Rules using Azure Active Directory (Azure AD) OAuth 2.0 client credentials flow. This allows secure transmission of log data to Azure Monitor workspaces.

## Features

- **Token Caching**: Minimizes authentication requests by caching access tokens and automatically refreshing them before expiry
- **Automatic Token Refresh**: Tokens are refreshed 5 minutes before expiration to prevent authentication failures
- **Thread-Safe**: Uses mutex protection for token cache operations
- **Error Handling**: Comprehensive error handling with detailed logging
- **JSON Response Parsing**: Uses json-c library for robust JSON response parsing

## Configuration

Add the `azure_auth()` block to your HTTP destination configuration:

```conf
destination d_azure_dcr {
    http(
        url("https://<data-collection-endpoint>/dataCollectionRules/<dcr-id>/streams/<stream-name>?api-version=2023-01-01")
        method("POST")
        headers("Content-Type: application/json")
        body('${MESSAGE}')
        
        azure_auth(
            tenant_id("<your-azure-tenant-id>")
            client_id("<your-client-app-id>")
            client_secret("<your-client-secret>")
            scope("https://monitor.azure.com/.default")  # Optional, this is the default
            auth_timeout(30)  # Optional, timeout in seconds, default is 30
        )
    );
};
```

### Configuration Parameters

- **tenant_id**: Your Azure AD tenant ID (required)
- **client_id**: The client (application) ID of your Azure AD app registration (required)
- **client_secret**: The client secret for your Azure AD app registration (required)
- **scope**: OAuth 2.0 scope for Azure Monitor (optional, defaults to "https://monitor.azure.com/.default")
- **auth_timeout**: Timeout for authentication requests in seconds (optional, defaults to 30)

## Azure Setup

### 1. Create an Azure AD App Registration

1. Go to Azure Portal → Azure Active Directory → App registrations
2. Click "New registration"
3. Provide a name (e.g., "syslog-ng-dcr-client")
4. Select "Accounts in this organizational directory only"
5. Click "Register"
6. Note the **Application (client) ID** and **Directory (tenant) ID**

### 2. Create a Client Secret

1. In your app registration, go to "Certificates & secrets"
2. Click "New client secret"
3. Add a description and set expiration
4. Click "Add"
5. **Copy the secret value immediately** (it won't be shown again)

### 3. Grant Permissions

1. Go to your Data Collection Rule in Azure Monitor
2. Click "Access control (IAM)"
3. Click "Add" → "Add role assignment"
4. Select "Monitoring Metrics Publisher" role
5. Assign to your Azure AD application

### 4. Get Data Collection Rule Information

You'll need:
- **Data Collection Endpoint URL**: Found in your DCE resource
- **Data Collection Rule ID**: Found in your DCR resource
- **Stream Name**: The custom log stream name you configured

## Example Complete Configuration

```conf
@version: 4.0

source s_system {
    system();
    internal();
};

# Transform logs to JSON format required by Azure DCR
rewrite r_azure_format {
    set('{"TimeGenerated": "${ISODATE}", "RawData": "${ESCAPECC}${MESSAGE}${ESCAPECC}", "Computer": "${HOST}"}', 
        value("AZURE_LOG"));
};

destination d_azure_dcr {
    http(
        url("https://my-dce-abc123.eastus-1.ingest.monitor.azure.com/dataCollectionRules/dcr-def456/streams/Custom-MyLogTable_CL?api-version=2023-01-01")
        method("POST")
        headers("Content-Type: application/json")
        body("${AZURE_LOG}")
        batch_lines(100)
        batch_timeout(10000)
        
        azure_auth(
            tenant_id("12345678-1234-1234-1234-123456789abc")
            client_id("87654321-4321-4321-4321-cba987654321")
            client_secret("your-client-secret-here")
        )
        
        # Optional TLS settings
        tls(
            peer_verify(yes)
            use_system_cert_store(yes)
        )
    );
};

log {
    source(s_system);
    rewrite(r_azure_format);
    destination(d_azure_dcr);
};
```

## Authentication Flow

1. **First Request**: When the first log message is sent, the module checks if a valid token exists
2. **Token Request**: If no valid token exists, it sends a POST request to `https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/token`
3. **Token Storage**: The received access token and expiration time are cached
4. **Token Usage**: The token is added as a Bearer authorization header to HTTP requests
5. **Token Refresh**: Tokens are automatically refreshed 5 minutes before expiry
6. **Error Handling**: If authentication fails, detailed error messages are logged

## Troubleshooting

### Common Issues

1. **Authentication Failed**: Check tenant_id, client_id, and client_secret
2. **Permission Denied**: Ensure the Azure AD app has "Monitoring Metrics Publisher" role on the DCR
3. **Invalid Scope**: Use "https://monitor.azure.com/.default" for Azure Monitor
4. **Network Issues**: Check firewall rules for outbound HTTPS to login.microsoftonline.com

### Debug Logging

Enable debug logging to see authentication details:

```bash
syslog-ng -F -d
```

Look for log messages with "Azure" in them for authentication status.

### Testing Authentication

You can test your Azure AD credentials using curl:

```bash
curl -X POST "https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/token" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=client_credentials&client_id={client_id}&client_secret={client_secret}&scope=https://monitor.azure.com/.default"
```

## Performance Considerations

- **Token Caching**: Tokens are cached to minimize authentication overhead
- **Batch Processing**: Use `batch_lines()` and `batch_timeout()` to reduce HTTP requests
- **Connection Reuse**: libcurl automatically reuses connections when possible
- **Error Recovery**: Failed authentication attempts trigger token refresh on next request

## Security Best Practices

1. **Store Secrets Securely**: Never hardcode client secrets in configuration files
2. **Use Environment Variables**: Consider using environment variables for sensitive data
3. **Rotate Secrets**: Regularly rotate client secrets in Azure AD
4. **Monitor Access**: Monitor authentication logs in Azure AD
5. **Network Security**: Use TLS and verify peer certificates

## Dependencies

- **libcurl**: For HTTP client functionality
- **json-c**: For JSON parsing of authentication responses
- **GLib**: For threading and memory management

## Building

The Azure DCR authentication feature is automatically built when the HTTP module is enabled and json-c is available:

```bash
cmake -DENABLE_CURL=ON ..
make
```

## Version Compatibility

This feature requires:
- syslog-ng 4.0 or later
- libcurl with HTTPS support
- json-c library
- Azure Monitor API version 2023-01-01 or later