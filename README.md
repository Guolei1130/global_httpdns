# global_httpdns

### 方案1

阿里云HttpDns上面的最佳实践。

缺陷:需要手动，比较麻烦，覆盖率低，webview问题大

### 方案2

动态代理Os接口

缺陷：怕厂商瞎修改，改个名字什么的。无法兼容webview、不兼容9.0，用了非公开API。

### 方案3

与方案二类似，但是放在native层，兼容9.0
用xHook，hook getaddrinfofornet(5.0以上)、getadrinfoforiface(4.4)

缺点：怕厂商瞎改。webview不兼容


### 方案4

方案3+inline hook libc.so 的getaddrinfo方法

缺点：怕厂商瞎改。
