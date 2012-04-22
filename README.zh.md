# MySQL Syncer
MySQL Syncer 是一套异构数据源复制的框架，基于解析MySQL binlog获取MySQL 操作数
据同步到任意数据源的服务。

参考：[MySQL Syncer](http://www.mysqlops.com/2012/04/18/%E5%BC%80%E6%BA%90%E9%A1%B9%E7%9B%AEmysql-syncer-%E7%AE%80%E4%BB%8B-%E5%BC%82%E6%9E%84%E6%95%B0%E6%8D%AE%E6%BA%90%E5%A4%8D%E5%88%B6.html)

## UPGRADING


## INSTALL
植入了Redis同步的一个简单Example，可以参考一下./test目录下的SQL脚本和PB。
* **`安装Redis`**:
    * 安装任何版本的Redis

* **`安装Probobuf`**:
    * tar xzf protobuf-2.4.0a.tar.gz -C 你的安装目录
    cd 安装目录/protobuf-2.4.0a
    ./configure
    make && make install

* **`安装Probobuf-C`**:
    * tar xzf probobuf-c-0.15.tar.gz -C 安装目录
    cd 安装目录/protobuf-c-0.15/
    echo /usr/local/lib > /etc/ld.so.conf.d/protobuf.conf
    ldconfig
    ./configure
    make && make install

* **`安装Redis-Client`**:
    * 步骤差不多，需要注意执行 
    echo /usr/local/lib > /etc/ld.so.conf.d/redis.conf
    ldconfig

* **`安装CUnit单元测试`**:
    * 解压之后 make && make install


* **`安装master`**:
    * make master_平台 (make master_64 or make master_32)
