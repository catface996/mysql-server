---
inclusion: fileMatch
fileMatchPattern: 'storage/sbt/**'
---

# MySQL TCP Connection Standard for SBT Storage Engine Testing

## Overview

This document establishes the mandatory standard for MySQL connections in all SBT storage engine testing and integration scenarios. All tests MUST use TCP connections exclusively.

## Connection Requirements

### Mandatory TCP Connection Parameters
All MySQL connections for SBT storage engine testing must use the following parameters:

```bash
# Required connection parameters
MYSQL_HOST="127.0.0.1"          # Always use IP address, not localhost
MYSQL_PORT="3306"               # Standard MySQL port
MYSQL_USER="root"               # Default user for testing
MYSQL_PASSWORD=""               # Empty password for development
MYSQL_DATABASE="test"           # Default test database
PROTOCOL="tcp"                  # Explicitly specify TCP protocol
```

### Connection Command Format
```bash
mysql -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} -D"$MYSQL_DATABASE" --protocol=tcp
```

### MySQL Configuration Requirements
The MySQL server configuration (`my.cnf`) must include:

```ini
[mysqld]
# TCP connection settings - MANDATORY
skip-networking=false           # Enable network connections
bind-address=127.0.0.1         # Bind to localhost IP
port=3306                      # Standard port

[client]
# Default client settings
port=3306
host=127.0.0.1
protocol=tcp                   # Force TCP protocol
```

## Prohibited Connection Methods

### NEVER Use These Methods
- ❌ **Socket connections**: `--socket=/path/to/socket`
- ❌ **Localhost hostname**: `--host=localhost` (use IP instead)
- ❌ **Default protocol**: Always specify `--protocol=tcp`
- ❌ **Named pipes**: Windows named pipe connections
- ❌ **Shared memory**: Windows shared memory connections

### Why TCP Only?
1. **Consistency**: TCP connections work identically across all platforms
2. **Reliability**: TCP connections are more predictable in CI/CD environments
3. **Debugging**: Network-based connections are easier to monitor and debug
4. **Isolation**: TCP connections provide better process isolation
5. **Compatibility**: Works with containerized and remote MySQL instances

## Implementation Standards

### Test Script Requirements
All test scripts in `storage/sbt/tests/mysql_integration/` must:

1. **Use TCP connection parameters** as defined above
2. **Validate connection** before running tests
3. **Handle connection failures** gracefully with clear error messages
4. **Document connection parameters** in script headers

### Example Implementation
```bash
#!/bin/bash

# SBT Storage Engine - MySQL Integration Test
# MANDATORY: Uses TCP connection only

# TCP connection parameters - DO NOT CHANGE
MYSQL_HOST="127.0.0.1"
MYSQL_PORT="3306"
MYSQL_USER="root"
MYSQL_PASSWORD=""
MYSQL_DATABASE="test"

# Function to execute MySQL command via TCP
execute_mysql() {
    local sql="$1"
    mysql -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
          ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
          -D"$MYSQL_DATABASE" \
          --protocol=tcp \
          -e "$sql"
}

# Validate TCP connection before tests
check_tcp_connection() {
    if execute_mysql "SELECT 1;" 2>/dev/null; then
        echo "✓ TCP connection successful"
        return 0
    else
        echo "✗ TCP connection failed"
        echo "Ensure MySQL server is running with TCP enabled"
        return 1
    fi
}
```

### Error Handling Standards
When TCP connection fails, tests must:

1. **Provide clear error message** indicating TCP connection failure
2. **Display connection parameters** being used
3. **Suggest troubleshooting steps**:
   - Check if MySQL server is running
   - Verify TCP is enabled (`skip-networking=false`)
   - Confirm port 3306 is accessible
   - Check firewall settings

### Example Error Message
```bash
print_error "TCP connection to MySQL failed"
print_error "Connection parameters:"
print_error "  Host: $MYSQL_HOST"
print_error "  Port: $MYSQL_PORT"
print_error "  User: $MYSQL_USER"
print_error "  Database: $MYSQL_DATABASE"
print_error "  Protocol: TCP"
print_error ""
print_error "Troubleshooting steps:"
print_error "1. Ensure MySQL server is running"
print_error "2. Check that skip-networking=false in my.cnf"
print_error "3. Verify port 3306 is accessible"
print_error "4. Check firewall settings"
```

## Testing Environment Setup

### MySQL Server Configuration
Before running any SBT storage engine tests:

1. **Verify MySQL configuration** includes TCP settings
2. **Restart MySQL server** if configuration changed
3. **Test TCP connectivity** using standard mysql client
4. **Confirm SBT plugin** is loaded and active

### Validation Commands
```bash
# 1. Check MySQL is running with TCP
netstat -an | grep :3306

# 2. Test TCP connection
mysql -h127.0.0.1 -P3306 -uroot --protocol=tcp -e "SELECT 1;"

# 3. Verify SBT engine is available
mysql -h127.0.0.1 -P3306 -uroot --protocol=tcp -e "SHOW ENGINES;" | grep SBT
```

## CI/CD Integration

### Continuous Integration Requirements
All CI/CD pipelines must:

1. **Configure MySQL with TCP enabled**
2. **Wait for TCP port availability** before running tests
3. **Use only TCP connections** in all test scripts
4. **Validate connection parameters** in pipeline logs

### Docker/Container Considerations
When using containerized MySQL:

```yaml
# Docker Compose example
services:
  mysql:
    image: mysql:8.0
    ports:
      - "3306:3306"  # Expose TCP port
    environment:
      MYSQL_ROOT_PASSWORD: ""
      MYSQL_ALLOW_EMPTY_PASSWORD: "yes"
    command: --skip-networking=false --bind-address=0.0.0.0
```

## Compliance Verification

### Before Task Completion
Every MySQL integration test must:

- ✅ Use TCP connection exclusively
- ✅ Include connection validation
- ✅ Handle TCP connection failures gracefully
- ✅ Document connection parameters
- ✅ Pass TCP connectivity test

### Code Review Checklist
- ✅ No socket connections used
- ✅ No localhost hostname used
- ✅ TCP protocol explicitly specified
- ✅ Connection parameters documented
- ✅ Error handling includes TCP troubleshooting

## Benefits of TCP-Only Standard

1. **Platform Independence**: Works on Linux, macOS, Windows
2. **Container Compatibility**: Essential for Docker/Kubernetes deployments
3. **Remote Testing**: Enables testing against remote MySQL instances
4. **Network Monitoring**: TCP connections can be monitored with network tools
5. **Security**: TCP connections support SSL/TLS encryption
6. **Scalability**: Supports connection pooling and load balancing

## Enforcement

This standard is **MANDATORY** for all SBT storage engine development:

- All existing tests must be updated to use TCP connections
- New tests must implement TCP connections from the start
- Code reviews must verify TCP compliance
- CI/CD pipelines must enforce TCP-only connections

## References

- MySQL Documentation: [Connecting to MySQL Server](https://dev.mysql.com/doc/refman/8.0/en/connecting.html)
- MySQL Configuration: [Server System Variables](https://dev.mysql.com/doc/refman/8.0/en/server-system-variables.html)
- Network Troubleshooting: [Connection Issues](https://dev.mysql.com/doc/refman/8.0/en/problems-connecting.html)

---

**Document Version**: 1.0  
**Last Updated**: 2025-07-26  
**Applies To**: All SBT storage engine testing and integration  
**Compliance**: Mandatory for all development and testing activities
