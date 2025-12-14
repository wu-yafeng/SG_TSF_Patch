# SG_TSF_Patch

QQ三国输入法智能感知插件。

## 背景

由于Microsoft的更新，自从Windows 8.1 以后，windows使用了一种称之为TSF的机制用于管理输入法，这导致QQ三国的原有使用的IME相关的API不在生效。

具体的表现是：当你在放技能的时候，可能会出现输入法，需要手动按SHIFT切换为英文状态。

所以这个插件的作用就是可以让你不再需要按shift切换输入法，完美兼容打字+游玩，就像在win7系统游玩QQ三国一样。

## 开始

在 [Release](https://github.com/wu-yafeng/SG_TSF_Patch/releases) 中下载二进制程序，并解压到一个全英文的目录，管理员身份运行 BootMain.exe。之后会自动打开QQ三国，并注入了输入法的补丁插件。

## 说明

该插件仅能用在Windows 8.1 及以上的系统，Windows7及以下不需要也不要使用此插件。

## 卸载

删除插件的目录即可。
