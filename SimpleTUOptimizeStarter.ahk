#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
#SingleInstance off
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.
FileCreateDir, %A_ScriptDir%\data

VersionOfStarter := "v2.64.0"

VersionOfTUO := ""
file = %A_temp%\version.tmp
RunWait, %comspec% /c "tuo.exe -version > %file%",, Hide
FileReadLine,VersionOfTUO,%file%,1
;VersionOfTUO := SubStr(VersionOfTUO, 28)
FileDelete, %file%

MaxCardsSections := 100

BGEffects := "none|SuperHeroism|Crackdown|Devotion|Unity|Iron-Will|Cold-Sleep|Blood-Vengeance|Oath-of-Loyalty|Furiosity|TemporalBacklash|CriticalReach|Devour|HaltedOrders|ZealotsPreservation|Virulence|Enfeeble all X|Enhance all S X|Evolve n S1 S2|Heal all X|Mortar X|Protect all X|Rally all X|Siege all X|Strike all X|Weaken all X|Brigade|Bloodlust X|Counterflux|Divert|EnduringRage|Fortification|Heroism|Metamorphosis|Megamorphosis|Revenge X|TurningTides"
IniFileName := "data\SimpleTUOptimizeStarter.ini"
IniSection := "onLoad"

IniRead, IniMyDeck, %IniFileName%, %IniSection%, MyDeck, Cyrus, Medic, Revolver, Imperial APC, Medic, Imperial APC
IniRead, IniMySiege, %IniFileName%, %IniSection%, MySiege, %A_Space%
IniRead, IniEnemiesDeck, %IniFileName%, %IniSection%, EnemiesDeck, Mission #94
IniRead, IniEnemySiege, %IniFileName%, %IniSection%, EnemySiege, %A_Space%
IniRead, IniVIP, %IniFileName%, %IniSection%, VIP, %A_Space%
IniRead, IniIterations, %IniFileName%, %IniSection%, Iterations, 10000
IniRead, IniThreads, %IniFileName%, %IniSection%, Threads, 4
IniRead, IniSimOptions, %IniFileName%, %IniSection%, SimOptions, %A_Space%
IniRead, IniEffect, %IniFileName%, %IniSection%, Effect, none
IniRead, IniMode, %IniFileName%, %IniSection%, Mode, 1
IniRead, IniOrder, %IniFileName%, %IniSection%, Order, 1
IniRead, IniOperation, %IniFileName%, %IniSection%, Operation, 1
IniRead, IniDominion, %IniFileName%, %IniSection%, Dominion, 1
IniRead, IniMono, %IniFileName%, %IniSection%, Mono, 1
IniRead, IniStrategy, %IniFileName%, %IniSection%, Strategy, 1
IniRead, IniCommander, %IniFileName%, %IniSection%, Commander, 0
IniRead, IniEndgame, %IniFileName%, %IniSection%, Endgame, 1
IniRead, IniFund, %IniFileName%, %IniSection%, Fund, 0
IniRead, Inix86, %IniFileName%, %IniSection%, x86, 0
IniRead, Inidebug, %IniFileName%, %IniSection%, debug, 0
IniRead, Iniopenmp, %IniFileName%, %IniSection%, openmp, 0
IniRead, Initime, %IniFileName%, %IniSection%, time, 0

Menu, MyMenu, Add, ownedcards.txt, MenuOwnedcards
Menu, MyMenu, Add, customdecks.txt, MenuCustomdecks
Menu, MyMenu, Add, cardabbrs.txt, MenuCardabbrs
Menu, MyMenu, Add, Update XMLs, MenuUpdate
Menu, MyMenu, Add, Update XMLs (DEV), MenuUpdateDev
Menu, MyMenu, Add, Help, MenuHelp
Menu, MyMenu, Add, Web, MenuWeb
Gui, Menu, MyMenu

Gui, Add, Text, r5, My Deck:
Gui, Add, Text, r1, My Fortress:
Gui, Add, Text, r5, Enemy Deck(s):
Gui, Add, Text, r1, Enemy Fortress:
Gui, Add, Text, r1, VIP Units:
Gui, Add, Text, r1, Quest:
Gui, Add, Text, r1, Effect:
Gui, Add, Text, r1, Mode:
Gui, Add, Text, r1, Operation:
Gui, Add, Text, r1, Dominion:
Gui, Add, Text, r1, Strategy:
Gui, Add, Text, r1, Flags:

Gui, Add, Edit, vMyDeck ym w600 r5, %IniMyDeck%
Gui, Add, Edit, vMySiege w600 r1, %IniMySiege%
Gui, Add, Edit, vEnemiesDeck w600 r5, %IniEnemiesDeck%
Gui, Add, Edit, vEnemySiege w600 r1, %IniEnemySiege%
Gui, Add, Edit, vVIP w600 r1, %IniVIP%
Gui, Add, DDL, vQuest w200 section, none||Activate X times of Berserk|Activate X times of Enfeeble|Activate X times of Flurry|Activate X times of Heal|Activate X times of Jam|Activate X times of Leech|Activate X times of Poison|Activate X times of Protect|Activate X times of Rally|Activate X times of Strike|Activate X times of Valor|Activate X times of Weaken|Deal X Counter damage|Deal X Poison damage|Destroy X Assault cards|Destroy X Structure cards|Play 3 Structure cards in each battle|Play 4 Bloodthirsty cards in each battle|Play 4 Imperial cards in each battle|Play 4 Raider cards in each battle|Play 4 Righteous cards in each battle|Play 4 Xeno cards in each battle
Gui, Add, Checkbox, vQuestWin ys Checked, and Win
Gui, Add, Text, ys, X=
Gui, Add, Edit, vQuestX number ys w30, 1

IniEffectNum := 0
BGEffectsArrayCount := 0

StringSplit, BGEffectsArray, BGEffects, |
Loop, %BGEffectsArray0%
{
    BGEffectsArrayCount += 1
    if ( BGEffectsArray%a_index% = IniEffect) {
		IniEffectNum := a_index
    }
}

if (IniEffectNum = 0) {
	IniEffectNum := BGEffectsArrayCount + 1
	BGEffects := BGEffects . "|" . IniEffect
}

Gui, Add, ComboBox, vEffect xs Choose%IniEffectNum% section, %BGEffects%
Gui, Add, DDL, altsubmit vMode Choose%IniMode%, Battle / Mission|Battle (defense)|GW|GW (defense)|Brawl|Brawl (defense)|Raid|Campaign|CQ / Surge
Gui, Add, DDL, altsubmit vOperation Group Choose%IniOperation% xs, Climb|Sim|Reorder|Climbex|Anneal|Debug|Climb_Forts|Genetic|Beam
Gui, Add, DDL, altsubmit vDominion Group Choose%IniDominion% xs, dom-owned|dom-maxed|dom-none
Gui, Add, DDL, altsubmit vStrategy Group Choose%IniStrategy%, none|recent|jam|mortar|siege|strike|sunder|weaken|enhance|evolve|heal|mend|overload|protect|rally|fortify|enrage|entrap|rush|mimic|armor|avenge|scavenge|corrosive|counter|evade|subdue|absorb|flying|payback|revenge|tribute|refresh|wall|barrier|coalition|legion|pierce|rupture|swipe|drain|venom|hunt|berserk|inhibit|sabotage|leech|poison|allegiance|flurry|valor|stasis|summon|bravery|enfeeble|mark|disease

Gui, Add, Text, ys, Endgame:
Gui, Add, Text, r1, Order:
Gui, Add, Text, r1, Iterations:
Gui, Add, Text, r1, Monofaction:

Gui, Add, DDL, altsubmit vEndgame ys Choose%IniEndgame%, none|0 - Maxed Units|1 - Maxed Fused|2 - Maxed Quads
Gui, Add, DDL, altsubmit vOrder Group Choose%IniOrder%, Random|Ordered (honor 3-card hand)|Flexible
Gui, Add, Edit, vIterations w100 r1, %IniIterations%
Gui, Add, DDL, altsubmit vMono Group Choose%IniMono%, none|imperial|raider|bloodthirsty|xeno|righteous|progenitor

Gui, Add, Text, ys, Fund:
Gui, Add, Text, r1,
Gui, Add, Text, , Threads:
;Gui, Add, Text, , Lock commander:

Gui, Add, Edit, vFund number r1 ys w50, %IniFund%
Gui, Add, Text, r1,
Gui, Add, Edit, vThreads number w20, %IniThreads%
Gui, Add, Checkbox, vCommander Checked%IniCommander%, Commander lock

Gui, Add, Edit, vSimOptions r1 xs w600, %IniSimOptions%
Gui, Add, Button, default r2 w100 x100 y+15 section, Simulate
Gui, Add, Checkbox, vx86 Checked%Inix86%, x86 (32-bit)
Gui, Add, Checkbox, vopenmp Checked%IniOpenMP%, openmp
Gui, Add, Button, r2 w100 ys xs+200, Exit
Gui, Add, Checkbox, vdebug Checked%IniDebug%, debug
Gui, Add, Checkbox, vtime Checked%IniTime%, time
Gui, Show,, Simple TUO Starter %VersionOfStarter% (version of %VersionOfTUO%)
return

ButtonSimulate:
Gui, Submit
MyDeck := RegExReplace(MyDeck, "\R")
MyDeck := RegExReplace(MyDeck, "(^\s*|\s*$)")

EnemiesDeck := RegExReplace(EnemiesDeck, "\R")
EnemiesDeck := RegExReplace(EnemiesDeck, "(^\s*|\s*$)")

SimOptions := RegExReplace(SimOptions, "(^\s*|\s*$)")

GuiControl, , Edit1, %MyDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
GuiControl, , Edit3, %EnemiesDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
GuiControl, , Edit11, %SimOptions% ; this will put the content of the variable in the editbox (edit11 is taken by the winspy)
Gui, Submit, NoHide ; save the changes and not hide the windows)

selTUO := "tuo" . (x86 ? "-x86" : "") . (openmp ? "-openmp" : "") . (debug ? "-debug" : (time ? "-time" : "")) . ".exe"
selMode := (Mode == 1 ? "pvp" : Mode == 2 ? "pvp-defense" : Mode == 3 ? "gw" : Mode == 4 ? "gw-defense" :Mode == 5 ? "brawl" : Mode == 6 ? "brawl-defense" : Mode == 7 ? "raid" : Mode == 8 ? "campaign" : "surge")
selOrder := (Order == 1 ? "random" : Order == 2 ? "ordered" : "flexible")
selOperation :=  (Operation == 1 ? "climb" : Operation == 2 ? "sim" : Operation == 3 ? "reorder": Operation == 4 ? "climbex" : Operation == 5 ? "anneal" : Operation==6 ? "debug sim" : Operation==7 ?"climb_forts" : Operation==8 ? "genetic":"beam")
selMySiege := (MySiege == "" ? "" : "yf """ MySiege """ ")
selEnemySiege := ( EnemySiege == "" ? "" : "ef """ EnemySiege """ ")
selVIP := ( VIP == "" ? "" : "vip """ VIP """ " )
StringReplace, Quest, Quest, %A_Space%X%A_Space%,%A_Space%%QuestX%%A_Space%
StringReplace, Quest, Quest, Activate, su
StringReplace, Quest, Quest, %A_Space%times%A_Space%of
StringReplace, Quest, Quest, Deal, sd
StringReplace, Quest, Quest, %A_Space%damage
StringReplace, Quest, Quest, Destroy, ck
StringReplace, Quest, Quest, %A_Space%cards
StringReplace, Quest, Quest, Play, cu
StringReplace, Quest, Quest, in%A_Space%each%A_Space%battle, each
selQuest := (Quest == "" || Quest == "none" ? "" : "quest """ Quest (QuestWin ? " win" : "") """ ")
selEffect := ( Effect == "" || Effect == "none" ? "" : "-e """ Effect """ ")
selThreads := ( Threads == "4" ? "" : "-t " Threads " ")
selSimOptions := ( SimOptions == "" ? "" : SimOptions " ")
EndgameVal := Endgame -2
selEndgame := (Endgame <= 1 ? "" : "endgame " EndgameVal " ")
selFund := (Fund == "" ? "" : "fund " Fund " ")
selDominion := (Dominion == 3 ? "dom-none " : Dominion == 1 ? "dom-owned " : Dominion == 2 ? "dom-maxed " : "")
selMono := (Mono == 1 ? "" : Mono == 2 ? "-m imperial " : Mono == 3 ? "-m raider " : Mono == 4 ? "-m bloodthirsty " : Mono == 5 ? "-m xeno " : Mono == 6 ? "-m righteous " : Mono == 7 ? "-m progenitor " : "")
selStrategy := (Strategy == 1 ? "" : Strategy == 2 ? "strategy recent " : Strategy == 3 ? "strategy jam " : Strategy == 4 ? "strategy mortar " : Strategy == 5 ? "strategy siege " : Strategy == 6 ? "strategy strike " : Strategy == 7 ? "strategy sunder " :  Strategy == 8 ? "strategy weaken " : Strategy == 9 ? "strategy enhance" :  Strategy == 10 ? "strategy evolve" :  Strategy == 11 ? "strategy heal" :  Strategy == 12 ? "strategy mend" :  Strategy == 13 ? "strategy overload" :  Strategy == 14 ? "strategy protect" :  Strategy == 15 ? "strategy rally" :  Strategy == 16 ? "strategy fortify" :  Strategy == 17 ? "strategy enrage" :  Strategy == 18 ? "strategy entrap" :  Strategy == 19 ? "strategy rush" :  Strategy == 20 ? "strategy mimic" :  Strategy == 21 ? "strategy armor" :  Strategy == 22 ? "strategy avenge" :  Strategy == 23 ? "strategy scavenge" : Strategy == 24 ? "strategy corrosive" : Strategy == 25 ? "strategy evade" : Strategy == 26 ? "strategy subdue" : Strategy == 27 ? "strategy absorb" : Strategy == 28 ? "strategy flying" : Strategy == 29 ? "strategy payback" : Strategy == 30 ? "strategy revenge" : Strategy == 31 ? "strategy tribute" : Strategy == 32 ? "strategy refresh" : Strategy == 33 ? "strategy wall" : Strategy == 34 ? "strategy barrier" : Strategy == 35 ? "strategy coalition" : Strategy == 36 ? "strategy legion" : Strategy == 37 ? "strategy pierce" : Strategy == 38 ? "strategy rupture" : Strategy == 39 ? "strategy swipe" : Strategy == 40 ? "strategy drain" : Strategy == 41 ? "strategy venom" : Strategy == 42 ? "strategy hunt" : Strategy == 43 ? "strategy berserk" : Strategy == 44 ? "strategy inhibit" : Strategy == 45 ? "strategy sabotage" : Strategy == 46 ? "strategy leech" : Strategy == 47 ? "strategy poison" : Strategy == 48 ? "strategy allegiance" : Strategy == 49 ? "strategy allegiance" : Strategy == 50 ? "strategy flurry" : Strategy == 51 ? "strategy valor" : Strategy == 52 ? "strategy stasis" : Strategy == 53 ? "strategy summon" : Strategy == 54 ? "strategy bravery" : Strategy == 55 ? "strategy enfeeble" : Strategy == 56 ? "strategy mark" : Strategy == 57 ? "strategy disease" :"")
selCommander := (Commander ? "-c " : "")
execString = %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selQuest%%selEffect%%selThreads%%selEndgame%%selFund%%selSimOptions%%selOperation% %Iterations% %selCommander%%selDominion%%selMono%%selStrategy%
Run, cmd.exe /c title TUOptimizeOutput && echo %execString% && %execString% & pause
Gui, Show
return

MenuHelp:
Gui, Submit
selTUO := "tuo" . (x86 ? "-x86" : "") . (openmp ? "-openmp" : "") . (debug ? "-debug" : (time ? "-time" : "")) . ".exe"
Run, cmd.exe /c title TUOptimizeOutput && echo %selTUO% && %selTUO% & pause
Gui, Show
return

MenuWeb:
Gui, Submit
Run https://github.com/APN-Pucky/tyrant_optimize/releases
Gui, Show
return

MenuUpdate:
MsgBox, 0, Update started, Updating XML files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, %MaxCardsSections%
{
	URL = http://mobile.tyrantonline.com/assets/cards_section_%A_Index%.xml
	CardsFile = data\cards_section_%A_Index%.xml
	if (!DownloadCards(URL, CardsFile))
		break
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return

MenuUpdateDev:
MsgBox, 0, Update started, Updating XML (DEV) files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, %MaxCardsSections%
{
	URL = http://mobile-dev.tyrantonline.com/assets/cards_section_%A_Index%.xml
	CardsFile = data\cards_section_%A_Index%.xml
	if (!DownloadCards(URL, CardsFile))
		break
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return


MenuOwnedcards:
Gui, Submit
Run, Notepad.exe data\ownedcards.txt
Gui, Show
return

MenuCustomdecks:
Gui, Submit
Run, Notepad.exe data\customdecks.txt
Gui, Show
return

MenuCardabbrs:
Gui, Submit
Run, Notepad.exe data\cardabbrs.txt
Gui, Show
return

GuiClose:
ButtonExit:
Gui, Submit
IniWrite, %MyDeck%, %IniFileName%, %IniSection%, MyDeck
IniWrite, %MySiege%, %IniFileName%, %IniSection%, MySiege
IniWrite, %EnemiesDeck%, %IniFileName%, %IniSection%, EnemiesDeck
IniWrite, %EnemySiege%, %IniFileName%, %IniSection%, EnemySiege
IniWrite, %VIP%, %IniFileName%, %IniSection%, VIP
IniWrite, %Effect%, %IniFileName%, %IniSection%, Effect
IniWrite, %Mode%, %IniFileName%, %IniSection%, Mode
IniWrite, %Order%, %IniFileName%, %IniSection%, Order
IniWrite, %Operation%, %IniFileName%, %IniSection%, Operation
IniWrite, %Dominion%, %IniFileName%, %IniSection%, Dominion
IniWrite, %Iterations%, %IniFileName%, %IniSection%, Iterations
IniWrite, %Mono%, %IniFileName%, %IniSection%, Mono
IniWrite, %Strategy%, %IniFileName%, %IniSection%, Strategy
IniWrite, %Commander%,  %IniFileName%, %IniSection%, Commander
IniWrite, %Threads%, %IniFileName%, %IniSection%, Threads
IniWrite, %SimOptions%, %IniFileName%, %IniSection%, SimOptions
IniWrite, %Endgame%, %IniFileName%, %IniSection%, Endgame
IniWrite, %Fund%, %IniFileName%, %IniSection%, Fund
IniWrite, %x86%, %IniFileName%, %IniSection%, x86
IniWrite, %debug%, %IniFileName%, %IniSection%, debug
IniWrite, %openmp%, %IniFileName%, %IniSection%, openmp
IniWrite, %time%, %IniFileName%, %IniSection%, time

while true
{
  IfWinExist, TUOptimizeOutput
      WinClose ; use the window found above
  else
      break
}
ExitApp

DownloadCards(url,file) {
	UrlDownloadToFile, *0 %url%, %file%
	if ErrorLevel
	{
		MsgBox, Error downloading %file%.
		had_error := true
	}
	else
	{
		FileReadLine,VAR1,%file%,3
		If InStr(VAR1, "File Not Found")
		{
			FileDelete, %file%
			return 0
		}
	}
	return 1
}
