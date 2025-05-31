# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	Crawl-PSX
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "Linley's Dungeon Crawl"
	HOMEPAGE_URL "https://github.com/forestbelton/crawl-psx"
)

include_directories(source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPSX -g -ggdb")
set(CXXFILES
    source/abl-show.cc
    source/abyss.cc
    source/acr.cc
    source/beam.cc
    source/chardump.cc
    source/cloud.cc
    source/command.cc
    source/debug.cc
    source/decks.cc
    source/delay.cc
    source/describe.cc
    source/direct.cc
    source/dungeon.cc
    source/effects.cc
    source/fight.cc
    source/files.cc
    source/food.cc
    source/hiscores.cc
    source/initfile.cc
    source/insult.cc
    source/invent.cc
    source/itemname.cc
    source/items.cc
    source/item_use.cc
    source/it_use2.cc
    source/it_use3.cc
    source/lev-pand.cc
    source/libutil.cc
    source/macro.cc
    source/maps.cc
    source/message.cc
    source/misc.cc
    source/mon-pick.cc
    source/monplace.cc
    source/monspeak.cc
    source/monstuff.cc
    source/mon-util.cc
    source/mstuff2.cc
    source/mutation.cc
    source/newgame.cc
    source/ouch.cc
    source/output.cc
    source/overmap.cc
    source/player.cc
	source/psx/curses.cc
	source/psx/psx.cc
	source/psx/util.cc
    source/randart.cc
    source/religion.cc
    source/shopping.cc
    source/skills2.cc
    source/skills.cc
    source/spells1.cc
    source/spells2.cc
    source/spells3.cc
    source/spells4.cc
    source/spl-book.cc
    source/spl-cast.cc
    source/spl-util.cc
    source/stuff.cc
    source/transfor.cc
    source/view.cc
    source/wpn-misc.cc
)

psn00bsdk_add_executable(crawl GPREL ${CXXFILES})

psn00bsdk_add_cd_image(
	iso      # Target name
	crawl    # Output file name (= crawl.bin + crawl.cue)
	iso.xml  # Path to config file
	DEPENDS crawl system.cnf
)
