# CODE 

#### 编码
UTF-8  

#### 命名规范
* 头文件尽量不写 #include
* 统一使用驼峰
* 模块内方法用小写字母开头，通用方法除外，譬如 sds,utils等
* 方法中，模块名后字母大写为公开，譬如 NTAddReplySds，小写则为不公开给其他模块
* 用大写缩写代替命名空间，譬如NT表示network
* "_" 表示隐藏、局部
* "r" 开头的结构体成员，表示该值不由本结构体负责释放，为 reference 的缩写
* "g_" 前缀表示全局变量
* "ui_" 表示ui模块内变量，以此类推其他模块
* "UI_" 表示ui模块内方法，以此类推其他模块
* typedef struct 以 _t 结尾 ，core下的基本数据类型可不遵守该约束
* struct 以 _s 结尾，core下的基本数据类型可不遵守该约束
* enum 以 _e 结尾


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
SV src/service
ST src/script
NT src/net
ET src/event
``` 
