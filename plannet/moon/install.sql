drop table if exists b_citizen;
create table b_citizen(
    citizen_id integer primary key autoincrement not null ,
    code varchar(32) unique, --由程序生成，全局唯一
    nickname varchar(64),
    email varchar(128),
    password varchar(32) -- md5(password+"_@traveller")
);
insert into b_citizen (code,nickname,email,password) values('0124', '月兔', 'j@ioctl.cc', 'a51e47f646375ab6bf5dd2c42d3e6181');
