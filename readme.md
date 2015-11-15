这里大致介绍一下:  

简介:   
traveller一个 All-In-One 的游戏，它集合了服务端和客户端，你可以创造一个星系，并设定游戏规则、管理星系、接纳其他玩家，你也可以驾驶一艘飞船穿梭于其他的星系，也就是连接其他玩家创造的星系。   

背景介绍:   
这是一个绝对自由的世界，你可以随手创造一个星系，或者驾驶飞船自由穿梭。  
  
主要游戏特点:     
2-1. 终端下，暂时只支持linux、mac（我是在mac底下开发的）     
2-2. 玩家可以通过编程，自己实现一个星系，然后和其他星系互相承认、互相移民      

技术概要:      
关键词:c lua sqlite ncurses redis     

3-1. 星系编写:用 lua 编写星系相关逻辑     

3-2. 网络:     
3-2-1. 我把redis的事件处理（主要兼容os部分）拿来用，并参考通信协议重新实现了一次       
3-2-2. 客户端、服务端同时并存     
  
3-3. 线程:运行后分两个线程，一个处理网络和事件，一个处理UI     
  
安装与运行:     
4-1. 首先要安装 libcurses，我安装到了 /usr/local/lib/libncurses.a，所以src/Makefile，里根据你安装的位置重新设置一下     
4-2. 进入 deps/lua ，然后 make macosx （根据你的操作系统make，譬如linux等）       
4-3. 进入 galaxies/gemini/server 然后按照install.sql 生成sqlite数据库文件sqlite.db      
4-4. 进入 src/ ，然后make   
4-5. 进入 src/ ，运行 ./traveller ../conf/gemini.conf ，这样就运行了，自己连自己，也就是说，自己开了一个双子座server，然后客户端连接这个server，你会看到一个地图（～～～很挫。。。别打脸。。。），自适应屏幕的   
   
开发进度:   
ui只做了个自适应屏幕的地图展示，vim的上下左右控制光标，在地图里跑来跑去   
打开 redis 客户端 ./redis-cli -p 1092 ，   
然后运行galaxies PUBCitizenLogin j@ioctl.cn traveller ，你会发现返回登录成功，   
如果你运行 galaxies PUBCitizenLogin j@ioctl.cn 123 ，你会得到 (error) 用户名或密码错误   
这个例子展示了，从redis客户端发出请求，server接受请求，并交给lua脚本处理，lua调用数据库，并向redis客户端推送消息。   
终上所述，要完成还前路漫漫。。。   

TODO:   
我之前的想法是，做一个自由世界，然后收集一些有意思的星系，终端下的模拟人生。。。。不过。。。你懂得。。。。扯倒蛋了。。。。   
因为感觉前路漫漫，所以想试试开放出来，看看能不能产生有意思的事情。。。   
