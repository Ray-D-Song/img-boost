## img-boost

高性能图片处理服务器，支持将任意格式图片转换为 WebP 格式并进行尺寸调整。

### 使用方式

```
https://img-boost.your-server.com?src={base64}&width=64&height=64&quality=80
```

### 参数说明

* `src`: 源图片路径（Base64 编码）
* `width`: 目标图片宽度（可选）
* `height`: 目标图片高度（可选）
* `quality`: 图片压缩质量，取值范围 0-100（可选）

### 说明

* 宽度和高度参数均为可选
* 仅指定宽度或高度时，将按原图比例自动计算另一维度
* 宽度和高度都未指定时，保持原图尺寸

### Author

[Ray-D-Song](https://github.com/ray-d-song)
