Name "Vulture-UnNetHack"
OutFile "vulture-UnNetHack-a.b.c_win32.exe"
InstallDir "$PROGRAMFILES\Vulture-UnNetHack"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR
  File ..\..\vulture-UnNetHack-build\binary\*
  SetOutPath $INSTDIR\config
  File ..\..\vulture-UnNetHack-build\binary\config\*
  SetOutPath $INSTDIR\graphics
  File ..\..\vulture-UnNetHack-build\binary\graphics\*
  SetOutPath $INSTDIR\tiles
  File ..\..\vulture-UnNetHack-build\binary\tiles\*
  SetOutPath $INSTDIR\manual
  File ..\..\vulture-UnNetHack-build\binary\manual\*
  SetOutPath $INSTDIR\manual\img
  File ..\..\vulture-UnNetHack-build\binary\manual\img\*
  SetOutPath $INSTDIR\sound
  File ..\..\vulture-UnNetHack-build\binary\sound\*
  SetOutPath $INSTDIR\music
  File ..\..\vulture-UnNetHack-build\binary\music\*
  SetOutPath $INSTDIR\fonts
  File ..\..\vulture-UnNetHack-build\binary\fonts\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-UnNetHack" "DisplayName" "Vulture-UnNetHack"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-UnNetHack" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-UnNetHack" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-UnNetHack" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture-UnNetHack.lnk" "$INSTDIR\Vulture-UnNetHack.exe" "" "$INSTDIR\Vulture-UnNetHack.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture-UnNetHack"
  CreateShortCut "$SMPROGRAMS\Vulture-UnNetHack\Vulture-UnNetHack.lnk" "$INSTDIR\Vulture-UnNetHack.exe" "" "$INSTDIR\Vulture-UnNetHack.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture-UnNetHack\Manual.lnk" "$INSTDIR\manual\index.html" "" "$INSTDIR\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture-UnNetHack\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture-UnNetHack\Uninstall Vulture-UnNetHack.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-UnNetHack"
  DeleteRegKey HKLM SOFTWARE\Vulture-UnNetHack
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
  Delete "$SMPROGRAMS\Vulture-UnNetHack\Vulture-UnNetHack.lnk"
  Delete "$SMPROGRAMS\Vulture-UnNetHack\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture-UnNetHack\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture-UnNetHack\Uninstall Vulture-UnNetHack.lnk"
  RMDir "$SMPROGRAMS\Vulture-UnNetHack"
  Delete "$DESKTOP\Vulture-UnNetHack.lnk"
SectionEnd
