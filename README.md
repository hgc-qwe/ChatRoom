# ChatRoom

## 软件要求

- Ubuntu 22.04+
- C++17
- GCC
- CMake
- MySQL
- Redis
- OpenSSL
- Protobuf
- spdlog

## 安装

### 安装编译工具

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    git
```

### 安装 OpenSSL

```bash
sudo apt install -y libssl-dev
```

### 安装 Protobuf

```bash
sudo apt install -y \
protobuf-compiler \
libprotobuf-dev
```

### 安装 MySQL

```bash
sudo apt install -y \
mysql-server \
libmysqlclient-dev
```

- 启动 MySQL：

```bash
mysql -u root -p
```

### 安装 Redis

```bash
sudo apt install -y redis-server
```

- 启动 Redis：

```bash
sudo systemctl start redis-server
```

## 配置 MySQL

### 登录 MySQL

```bash
sudo mysql
```

### 创建数据库

- 见sql/chatroom.sql

```sql
mysql -u root -p < sql/chatroom.sql
```

- 用户名：root
- 密码：123456
- 数据库：chatroom
- 端口：3306

## 邮件配置

- 如果需要使用注册验证码、验证码登录或密码找回功能：

1. 复制配置文件：

   cp config/email.conf.example config/email.conf

2. 编辑 config/email.conf：

   smtp_server=smtp.qq.com
   smtp_port=465
   email=你的邮箱@qq.com
   auth_code=你的邮箱授权码

3. 将其中的邮箱和授权码修改为自己的邮箱信息。

## 项目编译

```bash
cd ChatRoom
cmake -B build && cmake --build build
```

## 启动服务器

```bash
cd build
```

- 使用默认地址(127.0.0.1:8000)启动：

```bash
./Server
```

- 指定服务器 IP 和端口

```bash
./Server <IP> <PORT>
./Server 127.0.0.1 9000
```

## 启动客户端

```bash
cd ChatRoom/build
```

- 使用默认地址(127.0.0.1:8000)启动：

```bash
./Client
```

- 指定服务器 IP 和端口

```bash
./Client <IP> <PORT>
./Client 127.0.0.1 9000
```
