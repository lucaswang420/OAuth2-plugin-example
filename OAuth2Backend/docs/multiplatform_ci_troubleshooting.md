# Multi-Platform CI Troubleshooting Guide

## Common Issues and Solutions

### macOS Build Failures

**Issue:** linker errors about architecture mismatches
**Solution:** Ensure `CMAKE_OSX_ARCHITECTURES="x86_64"` is set in all CMake commands

**Issue:** OpenSSL not found
**Solution:** Verify Homebrew OpenSSL@1.1 is installed and `OPENSSL_ROOT_DIR` is set

### Windows Build Failures

**Issue:** Conan package conflicts
**Solution:** Clear Conan cache and rebuild: `rm -rf ~/.conan2`

**Issue:** MSVC environment not set up
**Solution:** Verify `windows-2022` runner has proper MSVC 2022 installation

### Linux Build Failures

**Issue:** System library dependencies missing
**Solution:** Check `apt-get install` step includes all required packages

**Issue:** Docker services not starting
**Solution:** Verify service health checks and port availability

### General Issues

**Issue:** Tests failing due to database connection
**Solution:** Check service readiness wait times and database initialization

**Issue:** Cache not hitting
**Solution:** Verify cache keys match dependency changes

## Debugging Tips

1. **Enable detailed logging:** Check "Build with detailed logging on failure" step
2. **Platform diagnostics:** Review platform-specific diagnostic steps output
3. **Cache statistics:** Monitor cache hit rates to optimize performance
4. **Artifact analysis:** Download test logs to identify root causes

## Performance Optimization

- **Drogon cache hit:** Should reduce build time from 10-15 minutes to 2-3 minutes
- **Conan cache (Windows):** Should reduce dependency installation time significantly
- **Parallel execution:** All platforms run simultaneously, total time = max(single platform time)

## Getting Help

- Check [GitHub Actions documentation](https://docs.github.com/en/actions)
- Review [Drogon Framework documentation](https://drogonframework.github.io/)
- Examine [workflow file](../../.github/workflows/ci-multiplatform.yml) for detailed configuration
