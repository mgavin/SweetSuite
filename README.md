# Instant Training
This is an Instant training mod for Rocket League, more specifically, Bakkesmod. It immediately puts the player in training, as soon as a match is over. Allows for map selection from the Bakkesmod F2 Plugins menu. 

# Installation

Download the latest release and extract it to `rocketleague\Binaries\Win32\bakkesmod\plugins`

If done correctly, `InstantTraining.dll` shoud be located in `rocketleague\Binaries\Win32\bakkesmod\plugins` and `instanttraining.set` should be located in `rocketleague\Binaries\Win32\bakkesmod\plugins\settings\`

Next, we will edit our default plugins. Open up `rocketleague\Binaries\Win32\bakkesmod\cfg\plugins.cfg` with notepad or a similar text editing application. Append `plugin load InstantTraining` to the bottom of the file. It should look something like this: 

![pluginconfig](images\pluginscfg.png)

Save the file.

# Usage

Now the plugin will be loaded everytime you load Bakkesmod. You can easily access and change the plugin's settings by hitting F2 to open the Bakkesmod menu, and go to the `Plugins` tab. There you will be able to edit the settings for the InstantTraining mod.

![pluginsettings](images\pluginsettings.png)

# Planned features

## Auto GG

Useful if you don't want to come off as a sourpuss for leaving instantly. Says GG in chat right before leaving.

## Game Modes

Don't want to freeplay Soccar? No problem, you'll be able to choose what type of game you want to play from a dropdown menu!

## Delay

Joining training way too fast? No problem, you will be able to set a custom delay to wait before joining training!

## Pause if

This will be an option to pause the plugin if you are in a certain type of game, i.e Rumble, Dropshot, Casual, Ranked, Private Match, etc.