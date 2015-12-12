drop table if exists b_citizen;
create table b_citizen(
    citizen_id integer primary key autoincrement not null ,
    charactor char(1) not null, -- 在ui上显示的字符
    code char(32) unique, -- 由程序生成，全局唯一
    nickname char(64),
    email char(128),
    password char(32) -- md5(password+"_@travller")
);
insert into b_citizen (code,charactor,nickname,email,password) values('0124', 'j', 'j', 'j@ioctl.cc', '98aaa4d9e581aaac90606d2a6236123c');
-- password: travller
