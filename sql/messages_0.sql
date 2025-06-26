/*
 Navicat Premium Dump SQL

 Source Server         : mysql-2.1
 Source Server Type    : MySQL
 Source Server Version : 80039 (8.0.39)
 Source Host           : localhost:3306
 Source Schema         : gateserver

 Target Server Type    : MySQL
 Target Server Version : 80039 (8.0.39)
 File Encoding         : 65001

 Date: 26/06/2025 19:12:38
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for messages_0
-- ----------------------------
DROP TABLE IF EXISTS `messages_0`;
CREATE TABLE `messages_0`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `session_id` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '会话 ID（如 \"1001_2002\"）',
  `sender_id` int NULL DEFAULT NULL COMMENT '发送者 ID',
  `receiver_id` int NULL DEFAULT NULL COMMENT '接收者 ID',
  `content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL COMMENT '消息内容或图片 URL',
  `msg_type` tinyint NULL DEFAULT NULL COMMENT '消息类型（1-文本，2-图片，3-文件）',
  `status` tinyint NULL DEFAULT NULL COMMENT '消息状态（0-在线已发放，1-离线未发送）',
  `marking` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL COMMENT '-- 该消息唯一标识 ',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 121 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
