Name "Vulture-Nethack"
OutFile "vulture-nethack-a.b.c_win32.exe"
InstallDir "$PROGRAMFILES\Vulture-Nethack"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR
  File ..\..\vulture-nethack-build\binary\*
  SetOutPath $INSTDIR\config
  File ..\..\vulture-nethack-build\binary\config\*
  SetOutPath $INSTDIR\graphics
  File ..\..\vulture-nethack-build\binary\graphics\*
  SetOutPath $INSTDIR\tiles
  File ..\..\vulture-nethack-build\binary\tiles\*
  SetOutPath $INSTDIR\manual
  File ..\..\vulture-nethack-build\binary\manual\*
  SetOutPath $INSTDIR\manual\img
  File ..\..\vulture-nethack-build\binary\manual\img\*
  SetOutPath $INSTDIR\sound
  File ..\..\vulture-nethack-build\binary\sound\*
  SetOutPath $INSTDIR\music
  File ..\..\vulture-nethack-build\binary\music\*
  SetOutPath $INSTDIR\fonts
  File ..\..\vulture-nethack-build\binary\fonts\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-NetHack" "DisplayName" "Vulture-Nethack"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-NetHack" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-NetHack" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-NetHack" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture-NetHack.lnk" "$INSTDIR\Vulture-NetHack.exe" "" "$INSTDIR\Vulture-NetHack.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture-NetHack"
  CreateShortCut "$SMPROGRAMS\Vulture-NetHack\Vulture-NetHack.lnk" "$INSTDIR\Vulture-NetHack.exe" "" "$INSTDIR\Vulture-NetHack.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture-NetHack\Manual.lnk" "$INSTDIR\manual\index.html" "" "$INSTDIR\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture-NetHack\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture-NetHack\Uninstall Vulture-NetHack.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-NetHack"
  DeleteRegKey HKLM SOFTWARE\Vulture-NetHack
  Delete $INSTDIR\config\*
  Delete $INSTDIR\graphics\*
  Delete $INSTDIR\manual\img\*
  Delete $INSTDIR\manual\*
  Delete $INSTDIR\tiles\*
  Delete $INSTDIR\sound\*
  Delete $INSTDIR\music\*
  Delete $INSTDIR\fonts\*
  Delete $INSTDIR\*
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\graphics"
  RMDir "$INSTDIR\manual\img"
  RMDir "$INSTDIR\manual"
  RMDir "$INSTDIR\tiles"
  RMDir "$INSTDIR\sound"
  RMDir "$INSTDIR\music"
  RMDir "$INSTDIR\fonts"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\Vulture-NetHack\Vulture-NetHack.lnk"
  Delete "$SMPROGRAMS\Vulture-NetHack\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture-NetHack\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture-NetHack\Uninstall Vulture-NetHack.lnk"
  RMDir "$SMPROGRAMS\Vulture-NetHack"
  Delete "$DESKTOP\Vulture-NetHack.lnk"
SectionEnd
