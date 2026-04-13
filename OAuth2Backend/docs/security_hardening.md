# Security Hardening Guide (Phase 22)

本文档详细说明了 OAuth2 服务的安全加固措施，包括速率限制 (Rate Limiting) 和 安全响应头 (Security Headers)。

## 1. 速率限制 (Rate Limiting)

为了防止暴力破解和 DoS 攻击，系统使用 Drogon 官方的 Hodor 插件实现应用层速率限制。

### 1.1 机制设计

* **插件**: `drogon::plugin::Hodor`
* **算法**: 令牌桶 (Token Bucket)
* **识别策略**:
    1. 优先读取 `X-Forwarded-For` 头（取第一个 IP）。
    2. 其次读取 `X-Real-IP` 头。
    3. 最后降级使用 `Peer IP`（直连 IP）。
    4. 白名单 IP（127.0.0.1, Docker 网络）完全跳过限制。

### 1.2 限制规则

| 接口 (Path) | 方法 | 全局限制 | 每IP限制 | 每用户限制 | 触发响应 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `/oauth2/login` | POST | 5000/min | **5/min** | 5/min | `429 Too Many Requests` |
| `/oauth2/token` | POST | 10000/min | **10/min** | 10/min | `429 Too Many Requests` |
| `/api/register` | POST | 5000/min | **5/min** | 5/min | `429 Too Many Requests` |
| *所有其他接口* | * | 1000/min | 60/min | 无限制 | - |

### 1.3 配置方式

Hodor 插件通过 `config.json` 配置，支持动态调整限制参数而无需重新编译。

详细配置参见 `docs/superpowers/specs/2026-04-13-hodor-rate-limiter-migration-design.md`。

---

## 2. 安全响应头 (Security Headers)

为了防御常见的 Web 攻击（如 MIME 嗅探、点击劫持），系统强制添加了现代 Web 安全头。

### 2.1 机制设计

使用 Drogon 的 **Global Post Handling Advice** 在所有 HTTP 响应（包括 404/500）中注入安全头。

### 2.2 响应头列表

| Header | Value | 作用 |
| :--- | :--- | :--- |
| `X-Content-Type-Options` | `nosniff` | 禁止浏览器猜测 MIME 类型，防止 XSS。 |
| `X-Frame-Options` | `SAMEORIGIN` | 仅允许同源站点嵌入 iframe，防止点击劫持。 |
| `Content-Security-Policy` | `default-src 'self'; frame-ancestors 'self';` | 限制资源加载源，基础 CSP 策略。 |
| `Strict-Transport-Security` | `max-age=31536000; includeSubDomains` | 强制 HTTPS 连接 (HSTS)。 |

### 2.3 验证

可以使用 `curl -I` 命令验证：

```bash
curl -I http://localhost:5555/
# Output should contain:
# x-frame-options: SAMEORIGIN
# x-content-type-options: nosniff
# ...
```
