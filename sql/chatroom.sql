CREATE DATABASE IF NOT EXISTS chatroom;
USE chatroom;
-- ============================================
-- 1. 用户表
-- ============================================
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(50) NOT NULL,
  `password` varchar(255) NOT NULL,
  `state` varchar(20) DEFAULT 'offline',
  `email` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `email` (`email`)
);

-- ============================================
-- 2. 好友表
-- ============================================
DROP TABLE IF EXISTS `friend`;
CREATE TABLE `friend` (
  `userid` int NOT NULL,
  `friendid` int NOT NULL,
  PRIMARY KEY (`userid`, `friendid`)
);

-- ============================================
-- 3. 好友申请表
-- ============================================
DROP TABLE IF EXISTS `friend_request`;
CREATE TABLE `friend_request` (
  `id` int NOT NULL AUTO_INCREMENT,
  `fromid` int NOT NULL,
  `toid` int NOT NULL,
  `status` int DEFAULT '0',
  PRIMARY KEY (`id`)
);

-- ============================================
-- 4. 群组表
-- ============================================
DROP TABLE IF EXISTS `allgroup`;
CREATE TABLE `allgroup` (
  `id` int NOT NULL AUTO_INCREMENT,
  `groupname` varchar(50) NOT NULL,
  `groupdesc` varchar(200) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

-- ============================================
-- 5. 群用户关系表
-- ============================================
DROP TABLE IF EXISTS `groupuser`;
CREATE TABLE `groupuser` (
  `userid` int NOT NULL,
  `groupid` int NOT NULL,
  `grouprole` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`userid`, `groupid`)
);

-- ============================================
-- 6. 群申请表
-- ============================================
DROP TABLE IF EXISTS `group_request`;
CREATE TABLE `group_request` (
  `id` int NOT NULL AUTO_INCREMENT,
  `groupid` int NOT NULL,
  `userid` int NOT NULL,
  `status` int DEFAULT '0',
  PRIMARY KEY (`id`)
);

-- ============================================
-- 7. 私聊消息表
-- ============================================
DROP TABLE IF EXISTS `message`;
CREATE TABLE `message` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `fromid` int NOT NULL,
  `toid` int NOT NULL,
  `msg` text NOT NULL,
  `sendtime` datetime DEFAULT CURRENT_TIMESTAMP,
  `status` int DEFAULT '0',
  `fromname` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

-- ============================================
-- 8. 群聊消息表
-- ============================================
DROP TABLE IF EXISTS `group_message`;
CREATE TABLE `group_message` (
  `id` int NOT NULL AUTO_INCREMENT,
  `groupid` int NOT NULL,
  `userid` int NOT NULL,
  `msg` text NOT NULL,
  `sendtime` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `username` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

-- ============================================
-- 9. 文件表
-- ============================================
DROP TABLE IF EXISTS `file`;
CREATE TABLE `file` (
  `fileid` varchar(128) NOT NULL,
  `fromid` int NOT NULL,
  `toid` int NOT NULL,
  `filename` varchar(255) DEFAULT NULL,
  `filesize` bigint DEFAULT NULL,
  `status` int DEFAULT '0',
  `fromname` varchar(50) DEFAULT NULL,
  `type` int DEFAULT '0',
  `groupid` int DEFAULT '0',
  PRIMARY KEY (`fileid`)
);

-- ============================================
-- 10. 黑名单表
-- ============================================
DROP TABLE IF EXISTS `blacklist`;
CREATE TABLE `blacklist` (
  `id` int NOT NULL AUTO_INCREMENT,
  `userid` int NOT NULL,
  `blackid` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_blacklist` (`userid`,`blackid`)
);