Name "Vulture's Claw"
OutFile "vultures-a.b.c-claw_win32-d.exe"
InstallDir "$PROGRAMFILES\Vulture's Claw"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR
  File ..\..\slashem\binary\*
  SetOutPath $INSTDIR\config
  File ..\..\slashem\binary\config\*
  SetOutPath $INSTDIR\graphics
  File ..\..\slashem\binary\graphics\*
  SetOutPath $INSTDIR\tiles
  File ..\..\slashem\binary\tiles\*
  SetOutPath $INSTDIR\manual
  File ..\..\slashem\binary\manual\*
  SetOutPath $INSTDIR\manual\img
  File ..\..\slashem\binary\manual\img\*
  SetOutPath $INSTDIR\sound
  File ..\..\slashem\binary\sound\*
  SetOutPath $INSTDIR\music
  File ..\..\slashem\binary\music\*
  SetOutPath $INSTDIR\fonts
  File ..\..\slashem\binary\fonts\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "DisplayName" "Vulture's Claw"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vulture-SlashEM" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture's Claw.lnk" "$INSTDIR\Vulture-SlashEM.exe" "" "$INSTDIR\Vulture-SlashEM.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture's Claw"
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Play Vulture's Claw.lnk" "$INSTDIR\Vulture-SlashEM.exe" "" "$INSTDIR\Vulture-SlashEM.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Manual.lnk" "$INSTDIR\manual\index.html" "" "$INSTDIR\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Uninstall Vulture's Claw.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
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
  Delete "$SMPROGRAMS\Vulture's Claw\Play Vulture's Claw.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Uninstall Vulture's Claw.lnk"
  RMDir "$SMPROGRAMS\Vulture's Claw"
  Delete "$DESKTOP\Vulture's Claw.lnk"
SectionEnd
