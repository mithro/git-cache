# GitHub Integration Testing Strategy

## Overview

Testing GitHub integration is challenging because it requires real API calls, authentication, and can have side effects (creating/modifying repositories). This document outlines a multi-layered testing approach to ensure the GitHub functionality continues to work correctly.

## Testing Layers

### 1. Unit Tests (No Network Required)
**Purpose**: Test GitHub API client logic without making actual API calls  
**Location**: `github_test.c` (basic functionality tests)  
**Coverage**:
- ✅ Client creation/destruction
- ✅ Input validation 
- ✅ URL parsing (`github_parse_repo_url`)
- ✅ Error handling
- ✅ Memory management

**Run with**: `make github-test-run`

### 2. Integration Tests (Network Required)
**Purpose**: Test actual GitHub API calls with authentication  
**Location**: `tests/run_github_tests.sh`  
**Requirements**: `GITHUB_TOKEN` environment variable  
**Coverage**:
- Token validation
- Repository information retrieval
- API error handling
- Rate limiting behavior

**Run with**: `GITHUB_TOKEN=ghp_xxx make github-test`

### 3. Fork Operation Tests (Side Effects)
**Purpose**: Test actual fork creation and management  
**Requirements**: 
- `GITHUB_TOKEN` with repo permissions
- `TEST_REPO_OWNER` and `TEST_REPO_NAME` environment variables
- Target repository that can be safely forked

**Coverage**:
- Fork creation in personal account
- Fork creation in organization (`mithro-mirrors`)
- Privacy setting changes
- Handling of existing forks
- Fork URL generation

**Run with**: 
```bash
export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TEST_REPO_OWNER=octocat
export TEST_REPO_NAME=Hello-World
./tests/run_github_tests.sh
```

### 4. End-to-End Integration Tests
**Purpose**: Test complete git-cache workflow with GitHub integration  
**Requirements**: Same as Fork Operation Tests  
**Coverage**:
- Complete clone workflow with forking
- Cache creation → Fork creation → Modifiable checkout
- Verification that modifiable checkout uses fork URL
- Push/pull operations to forked repository

## Testing Infrastructure

### Safe Test Repository Selection

To minimize side effects, we use well-known, forkable test repositories:

1. **Primary**: `octocat/Hello-World` - GitHub's official test repository
2. **Backup**: `github/gitignore` - Large repository good for testing
3. **Custom**: Create dedicated test repositories in `git-cache-testing` org

### Fork Cleanup Strategy

```bash
# Manual cleanup after testing
gh repo delete mithro-mirrors/octocat-Hello-World --confirm
gh repo delete username/Hello-World --confirm
```

### CI/CD Integration

**GitHub Actions Workflow** (`.github/workflows/test.yml`):

1. **Matrix Testing**: Test across multiple GitHub tokens (if available)
2. **Secret Management**: Use GitHub repository secrets for tokens
3. **Conditional Execution**: Only run fork tests if secrets are available
4. **Cleanup Jobs**: Automated cleanup of test forks after testing

### Rate Limiting Considerations

GitHub API has rate limits:
- **Authenticated**: 5000 requests/hour
- **Repository operations**: More strict limits
- **Fork operations**: Additional restrictions

**Mitigation strategies**:
- Use test tokens with higher limits
- Implement exponential backoff
- Cache test results when possible
- Run fork tests only when necessary

## Continuous Monitoring

### 1. Scheduled Testing
Run GitHub integration tests on a schedule:
```yaml
# .github/workflows/github-integration.yml
on:
  schedule:
    - cron: '0 6 * * *'  # Daily at 6 AM UTC
  workflow_dispatch:     # Manual trigger
```

### 2. Canary Testing
Before releases, run extended integration tests:
- Test with multiple repositories
- Test with different organization permissions
- Verify fork URL generation across scenarios

### 3. Integration Health Checks
Simple health check script that can be run frequently:
```bash
#!/bin/bash
# Quick GitHub API health check
curl -H "Authorization: token $GITHUB_TOKEN" \
     -s https://api.github.com/rate_limit | \
     jq '.resources.core.remaining'
```

## Test Data Management

### Mock Data for Unit Tests
For tests that don't require network:
```c
// Test data for URL parsing
static const struct {
    const char *url;
    const char *expected_owner;
    const char *expected_repo;
} test_urls[] = {
    {"https://github.com/user/repo.git", "user", "repo"},
    {"git@github.com:user/repo.git", "user", "repo"},
    // ... more test cases
};
```

### Test Repository Requirements
Ideal test repositories should be:
- Publicly accessible
- Forkable by test accounts
- Small in size (for fast cloning)
- Stable (not frequently deleted/renamed)
- Have clear ownership permissions

## Security Considerations

### Token Management
- Use fine-grained personal access tokens when available
- Limit token permissions to minimum required:
  - `repo` (for private repositories)
  - `public_repo` (for public repositories)
  - `admin:org` (only for organization testing)

### Test Isolation
- Use dedicated test organizations (`git-cache-testing`, `mithro-mirrors`)
- Prefix test repositories with identifiable names
- Set up cleanup automation
- Monitor for unexpected repository creation

### Secrets in CI
```yaml
env:
  GITHUB_TOKEN: ${{ secrets.GITHUB_TEST_TOKEN }}
  TEST_REPO_OWNER: ${{ secrets.TEST_REPO_OWNER }}
  TEST_REPO_NAME: ${{ secrets.TEST_REPO_NAME }}
```

## Regression Testing

### Critical Path Testing
Focus on these key scenarios:
1. **Fork Creation**: New fork in personal account
2. **Organization Fork**: Fork into `mithro-mirrors` organization  
3. **Existing Fork**: Handle when fork already exists
4. **Fork URL Usage**: Verify modifiable checkout uses correct URL
5. **Privacy Settings**: Verify fork privacy can be set
6. **Error Recovery**: Handle API failures gracefully

### Test Data Snapshots
Capture API responses for consistent testing:
```bash
# Capture real GitHub API responses for testing
curl -H "Authorization: token $GITHUB_TOKEN" \
     https://api.github.com/repos/octocat/Hello-World > test_data/hello_world_repo.json
```

## Performance Testing

### API Response Times
Monitor GitHub API performance:
```bash
# Test API response times
time curl -H "Authorization: token $GITHUB_TOKEN" \
          https://api.github.com/repos/octocat/Hello-World
```

### Fork Operation Timing
Measure complete fork workflow:
- Time to create fork
- Time to set privacy
- Time to clone from fork
- Total end-to-end time

## Failure Scenarios

### Common Failure Points
1. **Token Expiration**: Test with expired tokens
2. **Rate Limiting**: Test when rate limited
3. **Permission Denied**: Test with insufficient permissions
4. **Network Failures**: Test with network timeouts
5. **Repository Deletion**: Test when target repo is deleted
6. **Organization Permissions**: Test when can't fork to org

### Recovery Testing
Verify graceful degradation:
```bash
# Test with invalid token
GITHUB_TOKEN=invalid_token ./git-cache clone https://github.com/user/repo.git

# Test with rate-limited token
# (requires special rate-limited test token)

# Test with network issues
# (use network simulation tools)
```

## Recommended Testing Schedule

### Per Commit (CI/CD)
- Unit tests (no network)
- Basic integration tests (with token)
- Mock fork operations

### Daily (Scheduled)
- Full integration tests
- Real fork operations
- API health checks

### Pre-Release
- Extended fork testing
- Multiple repository types
- Organization permission testing
- Performance benchmarking

### Monthly
- Test token rotation
- Review test repository health
- Update test data snapshots
- Security audit of test infrastructure

## Monitoring and Alerting

### Key Metrics
- GitHub API success rate
- Fork operation success rate
- Average response times
- Rate limit consumption
- Test failure patterns

### Alert Conditions
- GitHub API success rate < 95%
- Fork operations failing > 2 consecutive runs
- API response times > 10 seconds
- Rate limit exceeded repeatedly
- Test repository becomes unavailable

This comprehensive testing strategy ensures the GitHub integration remains robust and catches regressions early while minimizing side effects and API usage costs.