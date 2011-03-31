Name "Vulture-SlashEM"
OutFile "vulture-SlashEM-a.b.c_win32.exe"
InstallDir "$PROGRAMFILES\Vulture-SlashEM"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR
  File ..\..\vulture-SlashEM-build\binary\*
  SetOutPath $INSTDIR\config
  File ..\..\vulture-SlashEM-build\binary\config\*
  SetOutPath $INSTDIR\graphics
  File ..\..\vulture-SlashEM-build\binary\graphics\*
  SetOutPath $INSTDIR\tiles
  File ..\..\vulture-SlashEM-build\binary\tiles\*
  SetOutPath $INSTDIR\manual
  File ..\..\vulture-SlashEM-build\binary\manual\*
  SetOutPath $INSTDIR\manual\img
  File ..\..\vulture-SlashEM-build\binary\manual\img\*
  SetOutPath $INSTDIR\sound
  File ..\..\vulture-SlashEM-build\binary\sound\*
  SetOutPath $INSTDIR\music
  File ..\..\vulture-SlashEM-build\binary\music\*
  SetOutPath $INSTDIR\fonts
  File ..\..\vulture-SlashEM-build\binary\fonts\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "DisplayName" "Vulture-SlashEM"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture-SlashEM.lnk" "$INSTDIR\Vulture-SlashEM.exe" "" "$INSTDIR\Vulture-SlashEM.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture-SlashEM"
  CreateShortCut "$SMPROGRAMS\Vulture-SlashEM\Vulture-SlashEM.lnk" "$INSTDIR\Vulture-SlashEM.exe" "" "$INSTDIR\Vulture-SlashEM.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture-SlashEM\Manual.lnk" "$INSTDIR\manual\index.html" "" "$INSTDIR\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture-SlashEM\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture-SlashEM\Uninstall Vulture-SlashEM.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM"
  DeleteRegKey HKLM SOFTWARE\Vulture-SlashEM
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
  Delete "$SMPROGRAMS\Vulture-SlashEM\Vulture-SlashEM.lnk"
  Delete "$SMPROGRAMS\Vulture-SlashEM\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture-SlashEM\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture-SlashEM\Uninstall Vulture-SlashEM.lnk"
  RMDir "$SMPROGRAMS\Vulture-SlashEM"
  Delete "$DESKTOP\Vulture-SlashEM.lnk"
SectionEnd
