-- Dreadmyst Database Setup Script (FIXED)
-- Run this in MySQL to create all required tables
-- All tables in ONE database: dreadmyst

-- Create database
CREATE DATABASE IF NOT EXISTS dreadmyst CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE dreadmyst;

-- Player table (added hash column)
CREATE TABLE IF NOT EXISTS `player` (
  `guid` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `hash` VARCHAR(64) DEFAULT NULL,
  `account` INT NOT NULL,  -- SIGNED: char delete soft-deletes by setting account = -accountId (server soft-delete); UNSIGNED rejected it ("Out of range value for column 'account'")
  `name` VARCHAR(32) NOT NULL,
  `class` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `gender` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `portrait` INT NOT NULL DEFAULT 0,
  `level` INT NOT NULL DEFAULT 1,
  `hp_pct` FLOAT NOT NULL DEFAULT 100,
  `mp_pct` FLOAT NOT NULL DEFAULT 100,
  `xp` INT NOT NULL DEFAULT 0,
  `money` INT NOT NULL DEFAULT 0,
  `position_x` FLOAT NOT NULL DEFAULT 0,
  `position_y` FLOAT NOT NULL DEFAULT 0,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  `map` INT NOT NULL DEFAULT 0,
  `home_map` INT NOT NULL DEFAULT 0,
  `home_x` FLOAT NOT NULL DEFAULT 0,
  `home_y` FLOAT NOT NULL DEFAULT 0,
  `logout_time` BIGINT NOT NULL DEFAULT 0,
  `last_death_time` BIGINT NOT NULL DEFAULT 0,
  `pvp_points` INT NOT NULL DEFAULT 0,
  `pk_count` INT NOT NULL DEFAULT 0,
  `pvp_flag` TINYINT NOT NULL DEFAULT 0,
  `progression` INT NOT NULL DEFAULT 0,
  `num_invested_spells` INT NOT NULL DEFAULT 0,
  `combat_rating` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`),
  UNIQUE KEY `name` (`name`)
);

-- Guild table (added hash column)
CREATE TABLE IF NOT EXISTS `guild` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(64) NOT NULL,
  `hash` VARCHAR(64) DEFAULT NULL,
  `leader_guid` INT UNSIGNED NOT NULL,
  `motd` TEXT,
  `create_date` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `guild_member` (
  `guid` INT UNSIGNED NOT NULL,
  `id` INT UNSIGNED NOT NULL,
  `rank` TINYINT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
);

CREATE TABLE IF NOT EXISTS `player_inventory` (
  `guid` INT UNSIGNED NOT NULL,
  `slot` INT NOT NULL,
  `entry` INT NOT NULL,
  `affix` INT NOT NULL DEFAULT 0,
  `affix_score` INT NOT NULL DEFAULT 0,
  `gem_1` INT NOT NULL DEFAULT 0,
  `gem_2` INT NOT NULL DEFAULT 0,
  `gem_3` INT NOT NULL DEFAULT 0,
  `gem_4` INT NOT NULL DEFAULT 0,
  `enchant_level` INT NOT NULL DEFAULT 0,
  `count` INT NOT NULL DEFAULT 1,
  `soulbound` TINYINT NOT NULL DEFAULT 0,
  `durability` INT NOT NULL DEFAULT 100,
  PRIMARY KEY (`guid`, `slot`)
);

CREATE TABLE IF NOT EXISTS `player_equipment` (
  `guid` INT UNSIGNED NOT NULL,
  `entry` INT NOT NULL,
  `affix` INT NOT NULL DEFAULT 0,
  `affix_score` INT NOT NULL DEFAULT 0,
  `gem_1` INT NOT NULL DEFAULT 0,
  `gem_2` INT NOT NULL DEFAULT 0,
  `gem_3` INT NOT NULL DEFAULT 0,
  `gem_4` INT NOT NULL DEFAULT 0,
  `enchant_level` INT NOT NULL DEFAULT 0,
  `soulbound` TINYINT NOT NULL DEFAULT 0,
  `durability` INT NOT NULL DEFAULT 100
);

CREATE TABLE IF NOT EXISTS `player_spell` (
  `guid` INT UNSIGNED NOT NULL,
  `spell_id` INT NOT NULL,
  `level` INT NOT NULL DEFAULT 1,
  PRIMARY KEY (`guid`, `spell_id`)
);

CREATE TABLE IF NOT EXISTS `player_quest_status` (
  `guid` INT UNSIGNED NOT NULL,
  `quest` INT NOT NULL,
  `rewarded` TINYINT NOT NULL DEFAULT 0,
  `objective_count1` INT NOT NULL DEFAULT 0,
  `objective_count2` INT NOT NULL DEFAULT 0,
  `objective_count3` INT NOT NULL DEFAULT 0,
  `objective_count4` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `quest`)
);

CREATE TABLE IF NOT EXISTS `player_bank` (
  `guid` INT UNSIGNED NOT NULL,
  `slot` INT NOT NULL,
  `entry` INT NOT NULL,
  `affix` INT NOT NULL DEFAULT 0,
  `affix_score` INT NOT NULL DEFAULT 0,
  `gem_1` INT NOT NULL DEFAULT 0,
  `gem_2` INT NOT NULL DEFAULT 0,
  `gem_3` INT NOT NULL DEFAULT 0,
  `gem_4` INT NOT NULL DEFAULT 0,
  `enchant_level` INT NOT NULL DEFAULT 0,
  `count` INT NOT NULL DEFAULT 1,
  `soulbound` TINYINT NOT NULL DEFAULT 0,
  `durability` INT NOT NULL DEFAULT 100,
  PRIMARY KEY (`guid`, `slot`)
);

CREATE TABLE IF NOT EXISTS `player_aura` (
  `guid` INT UNSIGNED NOT NULL,
  `caster_guid` INT UNSIGNED NOT NULL,
  `spell_id` INT NOT NULL,
  `miliseconds` INT NOT NULL DEFAULT 0,
  `m_stackCount` INT NOT NULL DEFAULT 1,
  `m_spellLevel` INT NOT NULL DEFAULT 1,
  `m_casterLevel` INT NOT NULL DEFAULT 1,
  `m_tickTotal_1` FLOAT DEFAULT 0,
  `m_tickAmount_1` FLOAT DEFAULT 0,
  `m_tickAmountTicked_1` FLOAT DEFAULT 0,
  `m_casterGuid_1` INT DEFAULT 0,
  `m_numTicks_1` INT DEFAULT 0,
  `m_numTicksCounter_1` INT DEFAULT 0,
  `m_tickTimer_1` INT DEFAULT 0,
  `m_tickIntervalMs_1` INT DEFAULT 0,
  `m_tickTotal_2` FLOAT DEFAULT 0,
  `m_tickAmount_2` FLOAT DEFAULT 0,
  `m_tickAmountTicked_2` FLOAT DEFAULT 0,
  `m_casterGuid_2` INT DEFAULT 0,
  `m_numTicks_2` INT DEFAULT 0,
  `m_numTicksCounter_2` INT DEFAULT 0,
  `m_tickTimer_2` INT DEFAULT 0,
  `m_tickIntervalMs_2` INT DEFAULT 0,
  `m_tickTotal_3` FLOAT DEFAULT 0,
  `m_tickAmount_3` FLOAT DEFAULT 0,
  `m_tickAmountTicked_3` FLOAT DEFAULT 0,
  `m_casterGuid_3` INT DEFAULT 0,
  `m_numTicks_3` INT DEFAULT 0,
  `m_numTicksCounter_3` INT DEFAULT 0,
  `m_tickTimer_3` INT DEFAULT 0,
  `m_tickIntervalMs_3` INT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS `player_spell_cooldown` (
  `guid` INT UNSIGNED NOT NULL,
  `spell_id` INT NOT NULL,
  `start_date` BIGINT NOT NULL,
  `end_date` BIGINT NOT NULL,
  PRIMARY KEY (`guid`, `spell_id`)
);

CREATE TABLE IF NOT EXISTS `player_waypoints` (
  `player_guid` INT UNSIGNED NOT NULL,
  `object_dbguid` INT NOT NULL
);

CREATE TABLE IF NOT EXISTS `player_stat_invest` (
  `guid` INT UNSIGNED NOT NULL,
  `stat_type` INT NOT NULL,
  `amount` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `stat_type`)
);

CREATE TABLE IF NOT EXISTS `player_mail` (
  `guid` INT UNSIGNED NOT NULL,
  `entry` INT NOT NULL,
  `affix` INT NOT NULL DEFAULT 0,
  `affix_score` INT NOT NULL DEFAULT 0,
  `count` INT NOT NULL DEFAULT 1,
  `gem_1` INT NOT NULL DEFAULT 0,
  `gem_2` INT NOT NULL DEFAULT 0,
  `gem_3` INT NOT NULL DEFAULT 0,
  `gem_4` INT NOT NULL DEFAULT 0,
  `enchant_level` INT NOT NULL DEFAULT 0,
  `soulbound` TINYINT NOT NULL DEFAULT 0,
  `durability` INT NOT NULL DEFAULT 100
);

CREATE TABLE IF NOT EXISTS `account_ignore` (
  `account_id` INT UNSIGNED NOT NULL,
  `target_name` VARCHAR(32) NOT NULL,
  PRIMARY KEY (`account_id`, `target_name`)
);

CREATE TABLE IF NOT EXISTS `account_played_time` (
  `id` INT UNSIGNED NOT NULL,
  `timeSecs` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `account_moderator` (
  `id` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `account_admin` (
  `id` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`)
);

-- Account fingerprints table (added hash column and DEFAULT for account_id)
CREATE TABLE IF NOT EXISTS `account_fingerprints` (
  `id` INT NOT NULL AUTO_INCREMENT,
  `account_id` INT NOT NULL DEFAULT 0,
  `hash` VARCHAR(64) DEFAULT NULL,
  `fingerprint` TEXT,
  `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `banned_accounts` (
  `id` INT UNSIGNED NOT NULL,
  `expire` DATETIME NOT NULL,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `account_ip` (
  `id` INT UNSIGNED NOT NULL,
  `ip_addr` VARCHAR(45) NOT NULL,
  PRIMARY KEY (`id`)
);

CREATE TABLE IF NOT EXISTS `logs` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` VARCHAR(32) NOT NULL,
  `message` TEXT,
  `timestamp` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
);

-- Auth tokens table (moved from dreadmyst_web to dreadmyst)
CREATE TABLE IF NOT EXISTS `auth_tokens` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` INT UNSIGNED NOT NULL,
  `token` VARCHAR(255) NOT NULL,
  `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `token` (`token`),
  KEY `user_id` (`user_id`)
);

-- Done!
SELECT 'Database setup complete! All tables created in dreadmyst database.' AS Status;
