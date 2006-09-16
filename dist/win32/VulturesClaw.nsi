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
  SetOutPath $INSTDIR\manual
  File ..\..\slashem\binary\manual\*
  SetOutPath $INSTDIR\sound
  File ..\..\slashem\binary\sound\*
  SetOutPath $INSTDIR\music
  File ..\..\slashem\binary\music\*

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VulturesClaw" "DisplayName" "Vulture's Claw"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VulturesClaw" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VulturesClaw" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VulturesClaw" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Desktop icon" SecDesktop
  SetOutPath $INSTDIR
  CreateShortcut "$DESKTOP\Vulture's Claw.lnk" "$INSTDIR\VulturesClaw.exe" "" "$INSTDIR\VulturesClaw.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\Vulture's Claw"
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Play Vulture's Claw.lnk" "$INSTDIR\VulturesClaw.exe" "" "$INSTDIR\VulturesClaw.exe" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Manual.lnk" "$INSTDIR\manual\index.html" "" "$INSTDIR\manual\index.html" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Darkarts Studios.lnk" "http://www.darkarts.co.za" "" "http://www.darkarts.co.za" 0
  CreateShortCut "$SMPROGRAMS\Vulture's Claw\Uninstall Vulture's Claw.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VulturesClaw"
  DeleteRegKey HKLM SOFTWARE\VulturesClaw
  Delete $INSTDIR\config\*
  Delete $INSTDIR\graphics\*
  Delete $INSTDIR\manual\*
  Delete $INSTDIR\sound\*
  Delete $INSTDIR\music\*
  Delete $INSTDIR\*
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\graphics"
  RMDir "$INSTDIR\manual"
  RMDir "$INSTDIR\sound"
  RMDir "$INSTDIR\music"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\Vulture's Claw\Play Vulture's Claw.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Manual.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Darkarts Studios.lnk"
  Delete "$SMPROGRAMS\Vulture's Claw\Uninstall Vulture's Claw.lnk"
  RMDir "$SMPROGRAMS\Vulture's Claw"
  Delete "$DESKTOP\Vulture's Claw.lnk"
SectionEnd
