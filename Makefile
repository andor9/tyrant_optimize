AHK2EXE="C:\Program Files\AutoHotkey\Compiler\Ahk2Exe.exe"
AHK32BIT="C:\Program Files\AutoHotkey\Compiler\ANSI 32-bit.bin"

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif
OS =windows #default=windows

ifeq ($(detected_OS),Windows)
    OS =windows
endif
ifeq ($(detected_OS),Darwin)  # Mac OS X
    OS =osx
endif
ifeq ($(detected_OS),Linux)  # Mac OS X
    OS =linux
endif
main:
		$(MAKE) -f make/Makefile.${OS} all

all: main debug test

test:
		$(MAKE) -f make/Makefile-test.${OS} all

debug:
		$(MAKE) -f make/Makefile-debug.${OS} all
ahk: starterahk livesimahk
livesimahk: SimpleTUOLiveSim.ahk
		$(AHK2EXE) /in SimpleTUOLiveSim.ahk /out SimpleTUOLiveSim.exe
		$(AHK2EXE) /in SimpleTUOLiveSim.ahk /out SimpleTUOLiveSim-x86.exe /bin $(AHK32BIT)
starterahk: SimpleTUOptimizeStarter.ahk
		$(AHK2EXE) /in SimpleTUOptimizeStarter.ahk /out SimpleTUOptimizeStarter.exe
		$(AHK2EXE) /in SimpleTUOptimizeStarter.ahk /out SimpleTUOptimizeStarter-x86.exe /bin $(AHK32BIT)
clean:
		$(MAKE) -f make/Makefile.${OS} clean
		$(MAKE) -f make/Makefile-debug.${OS} clean
		$(MAKE) -f make/Makefile-test.${OS} clean
commit:
		-git add .
		-git commit
push: commit
		git push

release-noahk: push
		git tag $(shell git describe --tags --abbrev=0 | perl -lpe 'BEGIN { sub inc { my ($$num) = @_; ++$$num } } s/(v\d+\.\d+\.)(\d+)/$$1 . (inc($$2))/eg')
		git push --tags

release: ahk release-noahk

pull: commit
		git pull
