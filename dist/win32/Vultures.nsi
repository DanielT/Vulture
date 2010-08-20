Name "Vulture's"
OutFile "vultures-a.b.c-full_win32-d.exe"
InstallDir "$PROGRAMFILES\Vulture's"

;--------------------------------

Page directory
Page instfiles

;--------------------------------

Section ""
  SetOutPath $INSTDIR\Eye
  File ..\..\nethack\binary\*
  SetOutPath $INSTDIR\Eye\config
  File ..\..\nethack\binary\config\*
  SetOutPath $INSTDIR\Eye\graphics
  File ..\..\nethack\binary\graphics\*
  SetOutPath $INSTDIR\Eye\manual
  File ..\..\nethack\binary\manual\*
  SetOutPath $INSTDIR\Eye\manual\img
  File ..\..\nethack\binary\manual\img\*
  SetOutPath $INSTDIR\Eye\sound
  File ..\..\nethack\binary\sound\*
  SetOutPath $INSTDIR\Eye\music
  File ..\..\nethack\binary\music\*
  SetOutPath $INSTDIR\Eye\fonts
  File ..\..\nethack\binary\fonts\*
  SetOutPath $INSTDIR\Claw
  File ..\..\slashem\binary\*
  SetOutPath $INSTDIR\Claw\config
  File ..\..\slashem\binary\config\*
  SetOutPath $INSTDIR\Claw\graphics
  File ..\..\slashem\binary\graphics\*
  SetOutPath $INSTDIR\Claw\manual
  File ..\..\slashem\binary\manual\*
  SetOutPath $INSTDIR\Claw\manual\img
  File ..\..\slashem\binary\manual\img\*
  SetOutPath $INSTDIR\Claw\sound
  File ..\..\slashem\binary\sound\*
  SetOutPath $INSTDIR\Claw\music
  File ..\..\slashem\binary\music\*
  SetOutPath $INSTDIR\Claw\fonts
  File ..\..\slashem\binary\fonts\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vultures" "DisplayName" "Vulture's"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vultures" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vultures" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vultures" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture's Eye.lnk" "$INSTDIR\Eye\Vulture-NetHack.exe" "" "$INSTDIR\Eye\Vulture-NetHack.exe" 0
  CreateShortcut "$DESKTOP\Vulture's Claw.lnk" "$INSTDIR\Claw\Vulture-SlashEM.exe" "" "$INSTDIR\Claw\Vulture-SlashEM.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture's"
  CreateShortCut "$SMPROGRAMS\Vulture's\Play Vulture's Eye.lnk" "$INSTDIR\Eye\Vulture-NetHack.exe" "" "$INSTDIR\Eye\Vulture-NetHack.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture's\Play Vulture's Claw.lnk" "$INSTDIR\Claw\Vulture-SlashEM.exe" "" "$INSTDIR\Claw\Vulture-SlashEM.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture's\Manual.lnk" "$INSTDIR\Eye\manual\index.html" "" "$INSTDIR\Eye\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture's\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture's\Uninstall Vulture's.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vultures"
  DeleteRegKey HKLM SOFTWARE\Vultures
  Delete $INSTDIR\Eye\config\*
  Delete $INSTDIR\Eye\graphics\*
  Delete $INSTDIR\Eye\manual\*
  Delete $INSTDIR\Eye\manual\img\*
  Delete $INSTDIR\Eye\sound\*
  Delete $INSTDIR\Eye\music\*
  Delete $INSTDIR\Eye\fonts\*
  Delete $INSTDIR\Eye\*
  RMDir "$INSTDIR\Eye\config"
  RMDir "$INSTDIR\Eye\graphics"
  RMDir "$INSTDIR\Eye\manual\img"
  RMDir "$INSTDIR\Eye\manual"
  RMDir "$INSTDIR\Eye\sound"
  RMDir "$INSTDIR\Eye"
  RMDir "$INSTDIR\Eye\music"
  RMDir "$INSTDIR\Eye\fonts"
  Delete $INSTDIR\Claw\config\*
  Delete $INSTDIR\Claw\graphics\*
  Delete $INSTDIR\Claw\manual\img\*
  Delete $INSTDIR\Claw\manual\*
  Delete $INSTDIR\Claw\sound\*
  Delete $INSTDIR\Claw\*
  Delete $INSTDIR\Claw\music\*
  Delete $INSTDIR\Claw\fonts\*
  RMDir "$INSTDIR\Claw\config"
  RMDir "$INSTDIR\Claw\graphics"
  RMDir "$INSTDIR\Claw\manual\img"
  RMDir "$INSTDIR\Claw\manual"
  RMDir "$INSTDIR\Claw\sound"
  RMDir "$INSTDIR\Claw"
  RMDir "$INSTDIR\Claw\music"
  RMDir "$INSTDIR\Claw\fonts"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\Vulture's\Play Vulture's Eye.lnk"
  Delete "$SMPROGRAMS\Vulture's\Play Vulture's Claw.lnk"
  Delete "$SMPROGRAMS\Vulture's\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture's\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture's\Uninstall Vulture's.lnk"
  RMDir "$SMPROGRAMS\Vulture's"
  Delete "$DESKTOP\Vulture's Eye.lnk"
  Delete "$DESKTOP\Vulture's Claw.lnk"
SectionEnd
