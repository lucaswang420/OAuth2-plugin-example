# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **BREAKING**: Migrate from custom RateLimiterFilter to Drogon's Hodor plugin
- Rate limiting now uses token bucket algorithm instead of fixed window
- Added per-user rate limiting support
- Added global rate limiting to protect server
- Added whitelist for local and Docker network IPs
- Rate limits now configurable via config.json (no code changes needed)

### Fixed

- Eliminated Redis dependency for rate limiting (now uses in-memory CacheMap)
- Improved rate limiting reliability (no single point of failure)
- Fixed IPv6 compatibility issues (removed IPv6 addresses from trust_ips)

### Technical Details

- **Removed**: `filters/RateLimiterFilter.cc`, `filters/RateLimiterFilter.h`
- **Added**: Hodor plugin configuration in `config.json`
- **Added**: User identification callback in `main.cc`
- **Updated**: Build script (`scripts/smart-build.bat`) to auto-detect Drogon build configuration
- **Migration**: Preserves existing rate limits while adding enhancements
- **Configuration**: Hodor supports multi-level limits (global, IP, user) via `config.json`

### Migration Notes

If you were using the old RateLimiterFilter:

1. Update your `config.json` to include Hodor plugin configuration
2. Remove any `RateLimiterFilter` references from your controllers
3. Review and adjust rate limits in `config.json` as needed
4. The new system provides more granular control and better performance

See [Hodor Migration Plan](docs/superpowers/plans/2026-04-13-hodor-rate-limiter-migration.md) for details.

---

## [Previous Versions]

For changes prior to this migration, please refer to git history.
