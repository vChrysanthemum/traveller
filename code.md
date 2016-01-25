# CODE 

#### 编码
UTF-8  

#### 命名规范
* 统一使用驼峰
* 模块内方法用小写字母开头，通用方法除外，譬如 sds,utils等
* 方法中，模块名后字母大写为公开，譬如 NTAddReplySds，小写则为不公开给其他模块
* 用大写缩写代替命名空间，譬如NT表示network
* "_" 表示隐藏、局部
* "g_" 前缀表示全局变量
* "ui_" 表示ui模块内变量，以此类推其他模块


#### 目录结构
```
code.md      					关于代码的解释
galaxies     					所有星系的配置、脚本、资源
galaxies/gemini					gemini的配置、脚本、资源
galaxies/gemini/client 			gemini客户端脚本、资源
galaxies/gemini/server 			gemini服务端脚本、资源
galaxies/gemini/gemini.conf 	gemini配置
tests							测试文件
doc								文档
src								traveller源代码
deps							第三方依赖，src/makefile中将会用到
readme.md
```

#### 模块   
```
UI src/ui 
UIHT src/ui/html 
SV src/service
ST src/script
NT src/net
ET src/event
``` 
