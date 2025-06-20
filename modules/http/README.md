http destination
================

The http destination can send the log as HTTP requests to an HTTP server.
It supports setting url, method, headers, user\_agent, authentication
and body. Only PUT and POST method is supported so far. If the method is
not set, POST will be used.

## Azure DCR Authentication

The HTTP destination now supports Azure Data Collection Rule (DCR) authentication for sending logs to Azure Monitor. This uses OAuth 2.0 client credentials flow with automatic token caching and refresh.

### Azure DCR Configuration Example:

```
@version: 4.0
@include "scl.conf"

source s_system { system(); internal(); };

destination d_azure_dcr {
    http(
        url("https://my-dce.eastus-1.ingest.monitor.azure.com/dataCollectionRules/dcr-12345/streams/Custom-MyLog_CL?api-version=2023-01-01")
        method("POST")
        headers("Content-Type: application/json")
        body('{"TimeGenerated": "${ISODATE}", "RawData": "${ESCAPECC}${MESSAGE}${ESCAPECC}", "Computer": "${HOST}"}')
        batch_lines(100)
        batch_timeout(10000)
        
        azure_auth(
            tenant_id("your-azure-tenant-id")
            client_id("your-azure-app-client-id") 
            client_secret("your-azure-app-secret")
            scope("https://monitor.azure.com/.default")  # Optional
            auth_timeout(30)  # Optional, seconds
        )
    );
};

log { source(s_system); destination(d_azure_dcr); };
```

### Azure DCR Authentication Options:

- **tenant_id()**: Azure AD tenant ID (required)
- **client_id()**: Azure AD application client ID (required)  
- **client_secret()**: Azure AD application secret (required)
- **scope()**: OAuth scope (optional, defaults to "https://monitor.azure.com/.default")
- **auth_timeout()**: Authentication request timeout in seconds (optional, defaults to 30)

## Basic HTTP Example:

```
@version: 3.28
@include "scl.conf"
source      s_system { system(); internal(); };
destination http_des {
    http(
        url("http://127.0.0.1:8000")
        method("PUT")
        user_agent("syslog-ng User Agent")
        user("user")
        password("password")
        headers("HEADER1: header1", "HEADER2: header2")
        body("${ISODATE} ${MSG}")
    );
};
log { source(s_system); destination(http_des); };
```

## Dependencies

The Azure DCR authentication feature requires:
- libcurl with HTTPS support
- json-c library for JSON response parsing
- Azure Monitor API version 2023-01-01 or later

See AZURE_DCR_README.md for detailed Azure setup instructions.
