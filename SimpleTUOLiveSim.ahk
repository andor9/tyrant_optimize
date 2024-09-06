#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
#SingleInstance off
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.
FileCreateDir, %A_ScriptDir%\data

VersionOfStarter := "v1.0.0"

VersionOfTUO := ""
file = %A_temp%\version.tmp
RunWait, %comspec% /c "tuo.exe -version > %file%",, Hide
FileReadLine,VersionOfTUO,%file%,1
;VersionOfTUO := SubStr(VersionOfTUO, 28)
FileDelete, %file%

MaxCardsSections := 100

BGEffects := "none|SuperHeroism|Crackdown|Devotion|Unity|Iron-Will|Cold-Sleep|Blood-Vengeance|Oath-of-Loyalty|Furiosity|TemporalBacklash|CriticalReach|Devour|HaltedOrders|ZealotsPreservation|Virulence|Enfeeble all X|Enhance all S X|Evolve n S1 S2|Heal all X|Mortar X|Protect all X|Rally all X|Siege all X|Strike all X|Weaken all X|Brigade|Bloodlust X|Counterflux|Divert|EnduringRage|Fortification|Heroism|Metamorphosis|Megamorphosis|Revenge X|TurningTides"
IniFileName := "data\SimpleTUOLiveSim.ini"
IniSection := "onLoad"

IniRead, IniMyDeck, %IniFileName%, %IniSection%, MyDeck, Cyrus, Medic, Revolver, Imperial APC, Medic, Imperial APC
IniRead, IniMySiege, %IniFileName%, %IniSection%, MySiege, %A_Space%
IniRead, IniMyPlayedCards, %IniFileName%, %IniSection%, MyPlayedCards,  Imperial APC, 
IniRead, IniMyHand1, %IniFileName%, %IniSection%, MyHand1,  Medic
IniRead, IniMyHand2, %IniFileName%, %IniSection%, MyHand2,  Revolver
IniRead, IniMyHand3, %IniFileName%, %IniSection%, MyHand3,  Imperial APC
IniRead, IniEnemiesDeck, %IniFileName%, %IniSection%, EnemiesDeck, Mission #94
IniRead, IniEnemySiege, %IniFileName%, %IniSection%, EnemySiege, %A_Space%
IniRead, IniEnemyPlayedCards, %IniFileName%, %IniSection%, EnemyPlayedCards, %A_Space%
IniRead, IniVIP, %IniFileName%, %IniSection%, VIP, %A_Space%
IniRead, IniIterations, %IniFileName%, %IniSection%, Iterations, 10000
IniRead, IniThreads, %IniFileName%, %IniSection%, Threads, 4
IniRead, IniSimOptions, %IniFileName%, %IniSection%, SimOptions, %A_Space%
IniRead, IniEffect, %IniFileName%, %IniSection%, Effect, none
IniRead, IniMode, %IniFileName%, %IniSection%, Mode, 1
IniRead, IniOrder, %IniFileName%, %IniSection%, Order, 2
IniRead, IniOperation, %IniFileName%, %IniSection%, Operation, 1
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
Gui, Add, Text, r1, My played Cards:
Gui, Add, Text, r1, My Hand 1:
Gui, Add, Text, r1, My Hand 2:
Gui, Add, Text, r1, My Hand 3:
Gui, Add, Text, r5, Enemy Deck(s):
Gui, Add, Text, r1, Enemy Fortress:
Gui, Add, Text, r1, Enemy played Cards:
Gui, Add, Text, r1, Effect:
Gui, Add, Text, r1, Mode:
Gui, Add, Text, r1, Operation:
Gui, Add, Text, r1, Flags:

Gui, Add, Edit, vMyDeck ym w600 r5, %IniMyDeck%
Gui, Add, Edit, vMySiege w600 r1, %IniMySiege%
Gui, Add, Edit, vMyPlayedCards w600 r1, %IniMyPlayedCards%
Gui, Add, Edit, vMyHand1 w300 r1 section, %IniMyHand1%
Gui, Add, Edit, vMyHand2 w300 r1, %IniMyHand2%
Gui, Add, Edit, vMyHand3 w300 r1, %IniMyHand3%
Gui, Add, Text, vMyHand1Score w300 r1 ys, 0
Gui, Add, Text, vMyHand2Score w300 r1 altsubmit, 0
Gui, Add, Text, vMyHand3Score w300 r1 altsubmit, 0
Gui, Add, Edit, vEnemiesDeck w600 r5 xs , %IniEnemiesDeck%
Gui, Add, Edit, vEnemySiege w600 r1, %IniEnemySiege%
Gui, Add, Edit, vEnemyPlayedCards w600 r1, %IniEnemyPlayedCards%

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

Gui, Add, ComboBox, vEffect Choose%IniEffectNum% section, %BGEffects%
Gui, Add, DDL, altsubmit vMode Choose%IniMode%, Battle / Mission|Battle (defense)|GW|GW (defense)|Brawl|Brawl (defense)|Raid|Campaign|CQ / Surge
Gui, Add, DDL, altsubmit vOperation Group Choose%IniOperation% xs, Reorder

Gui, Add, Text, ys r1, Order:
Gui, Add, Text, r1, Iterations:

Gui, Add, DDL, ys altsubmit vOrder Group Choose%IniOrder%, -R-a-n-d-o-m-|Ordered (honor 3-card hand)|-F-l-e-x-i-b-l-e-
Gui, Add, Edit, vIterations w100 r1, %IniIterations%

Gui, Add, Text,ys,Threads:

Gui, Add, Edit, ys vThreads number r1 w40, %IniThreads%

Gui, Add, Edit, vSimOptions r1 xs w600, %IniSimOptions%
Gui, Add, Button, default r2 w100 x150 y+15 section, Simulate
Gui, Add, Checkbox, vx86 Checked%Inix86%, x86 (32-bit)
Gui, Add, Checkbox, vopenmp Checked%IniOpenMP%, openmp
Gui, Add, Button, r2 w100 ys xs+200, Scores
Gui, Add, Checkbox, vdebug Checked%IniDebug%, debug
Gui, Add, Checkbox, vtime Checked%IniTime%, time
Gui, Add, Button, r2 w100 ys xs+400, Exit
Gui, Show,, Simple TUO LiveSim %VersionOfStarter% (version of %VersionOfTUO%)
return

ButtonSimulate:
Gui, Submit
selIterations := Iterations
MyDeck := RegExReplace(MyDeck, "\R")
MyDeck := RegExReplace(MyDeck, "(^\s*|\s*$)")

EnemiesDeck := RegExReplace(EnemiesDeck, "\R")
EnemiesDeck := RegExReplace(EnemiesDeck, "(^\s*|\s*$)")

SimOptions := RegExReplace(SimOptions, "(^\s*|\s*$)")

;GuiControl, , Edit1, %MyDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
;GuiControl, , Edit3, %EnemiesDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
;GuiControl, , Edit11, %SimOptions% ; this will put the content of the variable in the editbox (edit11 is taken by the winspy)
Gui, Submit, NoHide ; save the changes and not hide the windows)

selTUO := "tuo" . (x86 ? "-x86" : "") . (openmp ? "-openmp" : "") . (debug ? "-debug" : (time ? "-time" : "")) . ".exe"
selMode := (Mode == 1 ? "pvp" : Mode == 2 ? "pvp-defense" : Mode == 3 ? "gw" : Mode == 4 ? "gw-defense" :Mode == 5 ? "brawl" : Mode == 6 ? "brawl-defense" : Mode == 7 ? "raid" : Mode == 8 ? "campaign" : "surge")
selOrder := (Order == 1 ? "random" : Order == 2 ? "ordered" : "flexible")
selOperation :=  "reorder"
selMySiege := (MySiege == "" ? "" : "yf """ MySiege """ ")
selEnemySiege := ( EnemySiege == "" ? "" : "ef """ EnemySiege """ ")
selVIP := ( VIP == "" ? "" : "vip """ VIP """ " )
selEffect := ( Effect == "" || Effect == "none" ? "" : "-e """ Effect """ ")
selThreads := ( Threads == "4" ? "" : "-t " Threads " ")
selSimOptions := ( SimOptions == "" ? "" : SimOptions " ")
StringReplace MyPlayedCards, MyPlayedCards,`,,`,,UseErrorLevel
Turn := ErrorLevel + 1
selTurn := "freeze " Turn " "
selOpMyHand := "hand "
selOpenemyHand := "enemy:hand "
selMyHand = %selOpMyHand% "%MyPlayedCards%, %MyHand1%, %MyHand2%, %MyHand3%"
selEnemyHand = %selOpEnemyHand% "%EnemyPlayedCards%"
execString = %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selEffect%%selThreads%%selTurn%%selSimOptions%%selOperation% %selIterations% %selMyHand% %selEnemyHand%
Run, cmd.exe /c title TUOptimizeOutput && echo %execString% && %execString% & pause
Gui, Show
return

; ----------------------------------------------------------------------------------------------------------------------
; Function .....: StdoutToVar_CreateProcess
; Description ..: Runs a command line program and returns its output.
; Parameters ...: sCmd      - Commandline to execute.
; ..............: sEncoding - Encoding used by the target process. Look at StrGet() for possible values.
; ..............: sDir      - Working directory.
; ..............: nExitCode - Process exit code, receive it as a byref parameter.
; Return .......: Command output as a string on success, empty string on error.
; AHK Version ..: AHK_L x32/64 Unicode/ANSI
; Author .......: Sean (http://goo.gl/o3VCO8), modified by nfl and by Cyruz
; License ......: WTFPL - http://www.wtfpl.net/txt/copying/
; Changelog ....: Feb. 20, 2007 - Sean version.
; ..............: Sep. 21, 2011 - nfl version.
; ..............: Nov. 27, 2013 - Cyruz version (code refactored and exit code).
; ..............: Mar. 09, 2014 - Removed input, doesn't seem reliable. Some code improvements.
; ..............: Mar. 16, 2014 - Added encoding parameter as pointed out by lexikos.
; ..............: Jun. 02, 2014 - Corrected exit code error.
; ..............: Nov. 02, 2016 - Fixed blocking behavior due to ReadFile thanks to PeekNamedPipe.
; ----------------------------------------------------------------------------------------------------------------------
StdoutToVar_CreateProcess(sCmd, sEncoding:="CP0", sDir:="", ByRef nExitCode:=0) {
    DllCall( "CreatePipe",           PtrP,hStdOutRd, PtrP,hStdOutWr, Ptr,0, UInt,0 )
    DllCall( "SetHandleInformation", Ptr,hStdOutWr, UInt,1, UInt,1                 )

            VarSetCapacity( pi, (A_PtrSize == 4) ? 16 : 24,  0 )
    siSz := VarSetCapacity( si, (A_PtrSize == 4) ? 68 : 104, 0 )
    NumPut( siSz,      si,  0,                          "UInt" )
    NumPut( 0x100,     si,  (A_PtrSize == 4) ? 44 : 60, "UInt" )
    NumPut( hStdOutWr, si,  (A_PtrSize == 4) ? 60 : 88, "Ptr"  )
    NumPut( hStdOutWr, si,  (A_PtrSize == 4) ? 64 : 96, "Ptr"  )

    If ( !DllCall( "CreateProcess", Ptr,0, Ptr,&sCmd, Ptr,0, Ptr,0, Int,True, UInt,0x08000000
                                  , Ptr,0, Ptr,sDir?&sDir:0, Ptr,&si, Ptr,&pi ) )
        Return ""
      , DllCall( "CloseHandle", Ptr,hStdOutWr )
      , DllCall( "CloseHandle", Ptr,hStdOutRd )

    DllCall( "CloseHandle", Ptr,hStdOutWr ) ; The write pipe must be closed before reading the stdout.
    While ( 1 )
    { ; Before reading, we check if the pipe has been written to, so we avoid freezings.
        If ( !DllCall( "PeekNamedPipe", Ptr,hStdOutRd, Ptr,0, UInt,0, Ptr,0, UIntP,nTot, Ptr,0 ) )
            Break
        If ( !nTot )
        { ; If the pipe buffer is empty, sleep and continue checking.
            Sleep, 100
            Continue
        } ; Pipe buffer is not empty, so we can read it.
        VarSetCapacity(sTemp, nTot+1)
        DllCall( "ReadFile", Ptr,hStdOutRd, Ptr,&sTemp, UInt,nTot, PtrP,nSize, Ptr,0 )
        sOutput .= StrGet(&sTemp, nSize, sEncoding)
    }
    
    ; * SKAN has managed the exit code through SetLastError.
    DllCall( "GetExitCodeProcess", Ptr,NumGet(pi,0), UIntP,nExitCode )
    DllCall( "CloseHandle",        Ptr,NumGet(pi,0)                  )
    DllCall( "CloseHandle",        Ptr,NumGet(pi,A_PtrSize)          )
    DllCall( "CloseHandle",        Ptr,hStdOutRd                     )
    Return sOutput
}

ButtonScores:
Gui, Submit
selIterations := Iterations
MyDeck := RegExReplace(MyDeck, "\R")
MyDeck := RegExReplace(MyDeck, "(^\s*|\s*$)")

EnemiesDeck := RegExReplace(EnemiesDeck, "\R")
EnemiesDeck := RegExReplace(EnemiesDeck, "(^\s*|\s*$)")

SimOptions := RegExReplace(SimOptions, "(^\s*|\s*$)")

;GuiControl, , Edit1, %MyDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
;GuiControl, , Edit3, %EnemiesDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
;GuiControl, , Edit11, %SimOptions% ; this will put the content of the variable in the editbox (edit11 is taken by the winspy)
Gui, Submit, NoHide ; save the changes and not hide the windows)

selTUO := "tuo" . (x86 ? "-x86" : "") . (openmp ? "-openmp" : "") . (debug ? "-debug" : (time ? "-time" : "")) . ".exe"
selMode := (Mode == 1 ? "pvp" : Mode == 2 ? "pvp-defense" : Mode == 3 ? "gw" : Mode == 4 ? "gw-defense" :Mode == 5 ? "brawl" : Mode == 6 ? "brawl-defense" : Mode == 7 ? "raid" : Mode == 8 ? "campaign" : "surge")
selOrder := (Order == 1 ? "random" : Order == 2 ? "ordered" : "flexible")
selOperation :=  "sim"
selMySiege := (MySiege == "" ? "" : "yf """ MySiege """ ")
selEnemySiege := ( EnemySiege == "" ? "" : "ef """ EnemySiege """ ")
selVIP := ( VIP == "" ? "" : "vip """ VIP """ " )
selEffect := ( Effect == "" || Effect == "none" ? "" : "-e """ Effect """ ")
selThreads := ( Threads == "4" ? "" : "-t " Threads " ")
selSimOptions := ( SimOptions == "" ? "" : SimOptions " ")
StringReplace MyPlayedCards, MyPlayedCards,`,,`,,UseErrorLevel
Turn := ErrorLevel + 2
selTurn := "freeze " Turn " "
selOpMyHand := "hand "
selOpenemyHand := "enemy:hand "
selMyHand1 = %selOpMyHand% "%MyPlayedCards%, %MyHand1%"
selMyHand2 = %selOpMyHand% "%MyPlayedCards%, %MyHand2%"
selMyHand3 = %selOpMyHand% "%MyPlayedCards%, %MyHand3%"
selEnemyHand = %selOpEnemyHand% "%EnemyPlayedCards%"
execString1 =  %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selEffect%%selThreads%%selTurn%%selSimOptions%%selOperation% %selIterations% %selMyHand1% %selEnemyHand%
execString2 =  %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selEffect%%selThreads%%selTurn%%selSimOptions%%selOperation% %selIterations% %selMyHand2% %selEnemyHand%
execString3 =  %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selEffect%%selThreads%%selTurn%%selSimOptions%%selOperation% %selIterations% %selMyHand3% %selEnemyHand%
Test1 := StdoutToVar_CreateProcess( execString1 )
Test2 := StdoutToVar_CreateProcess( execString2 )
Test3 := StdoutToVar_CreateProcess( execString3 )
pp1 := StrSplit(StrSplit(Test1, "win`%:")[2], "(")[1]
pp2 := StrSplit(StrSplit(Test2, "win`%:")[2], "(")[1]
pp3 := StrSplit(StrSplit(Test3, "win`%:")[2], "(")[1]
MsgBox , %Test1%
GuiControl, , MyHand1Score, %pp1%
GuiControl, , MyHand2Score, %pp2%
GuiControl, , MyHand3Score, %pp3%
Gui, Submit, NoHide 
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
IniWrite, %MyPlayedCards%, %IniFileName%, %IniSection%, MyPlayedCards
IniWrite, %MyHand1%, %IniFileName%, %IniSection%, MyHand1
IniWrite, %MyHand2%, %IniFileName%, %IniSection%, MyHand2
IniWrite, %MyHand3%, %IniFileName%, %IniSection%, MyHand3
IniWrite, %EnemiesDeck%, %IniFileName%, %IniSection%, EnemiesDeck
IniWrite, %EnemySiege%, %IniFileName%, %IniSection%, EnemySiege
IniWrite, %EnemyPlayedCards%, %IniFileName%, %IniSection%, EnemyPlayedCards
IniWrite, %VIP%, %IniFileName%, %IniSection%, VIP
IniWrite, %Effect%, %IniFileName%, %IniSection%, Effect
IniWrite, %Mode%, %IniFileName%, %IniSection%, Mode
IniWrite, %Order%, %IniFileName%, %IniSection%, Order
IniWrite, %Operation%, %IniFileName%, %IniSection%, Operation
IniWrite, %Iterations%, %IniFileName%, %IniSection%, Iterations
IniWrite, %Threads%, %IniFileName%, %IniSection%, Threads
IniWrite, %SimOptions%, %IniFileName%, %IniSection%, SimOptions
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

