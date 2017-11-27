/******************************************************************************
MQ2Main.dll: MacroQuest2's extension DLL for EverQuest
Copyright (C) 2002-2003 Plazmic, 2003-2005 Lax

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
******************************************************************************/
#ifdef PRIVATE
#if defined(LIVE)
#include "eqgame-private(live).h"
#endif
#endif

#define __ClientName                                              "eqgame"
#define __ExpectedVersionDate                                     "Nov 27 2017"
#define __ExpectedVersionTime                                     "04:15:36"
#define __ActualVersionDate_x                                      0xAE40E0
#define __ActualVersionTime_x                                      0xAE40EC

#define __ClientOverride                                           0
#define __MacroQuestWinClassName                                  "__MacroQuest2Tray(Live)"
#define __MacroQuestWinName                                       "MacroQuest2(Live)"

// Memory Protection
#define __MemChecker0_x                                            0x5AB210
#define __MemChecker1_x                                            0x8A3C40
#define __MemChecker2_x                                            0x641430
#define __MemChecker3_x                                            0x641380
#define __MemChecker4_x                                            0x7F5A60
#define __EncryptPad0_x                                            0xC3EFE8
#define __EncryptPad1_x                                            0xD18BA0
#define __EncryptPad2_x                                            0xC67ED0
#define __EncryptPad3_x                                            0xC67AD0
#define __EncryptPad4_x                                            0xD071A0
#define __EncryptPad5_x                                            0x10C22C0
#define __AC1_x                                                    0x7B4B85
#define __AC2_x                                                    0x5643E7
#define __AC3_x                                                    0x562560
#define __AC4_x                                                    0x574731
#define __AC5_x                                                    0x58848B
#define __AC6_x                                                    0x58C03E
#define __AC7_x                                                    0x582A8C
#define __AC1_Data                                                 0x11111111
#define __EP1_Data_x                                               0x1FA4920

// Direct Input
#define DI8__Main_x                                                0x10C2298
#define DI8__Keyboard_x                                            0x10C229C
#define DI8__Mouse_x                                               0x10C22A0
#define DI8__Mouse_Copy_x                                          0xF30454
#define DI8__Mouse_Check_x                                         0xFB9F84
#define __AutoSkillArray_x                                         0xF31368
#define __Attack_x                                                 0xFB4A5B
#define __Autofire_x                                               0xFB4A5C
#define __BindList_x                                               0xC1B368
#define g_eqCommandStates_x                                        0xF1F1A0
#define __Clicks_x                                                 0xF30510
#define __CommandList_x                                            0xC1DA80
#define __CurrentMapLabel_x                                        0x10C8290
#define __CurrentSocial_x                                          0xBEA8E8
#define __DoAbilityList_x                                          0xF66C20
#define __do_loot_x                                                0x533310
#define __DrawHandler_x                                            0x1731F0C
#define __GroupCount_x                                             0xF20AAA

#define __Guilds_x                                                 0xF266C0
#define __gWorld_x                                                 0xF22F64
#define __HotkeyPage_x                                             0xFAD620
#define __HWnd_x                                                   0xFBA0A0
#define __InChatMode_x                                             0xF3043C
#define __LastTell_x                                               0xF3231C
#define __LMouseHeldTime_x                                         0xF3057C
#define __Mouse_x                                                  0x10C22A4
#define __MouseLook_x                                              0xF304D6
#define __MouseEventTime_x                                         0xFB51F4
#define __gpbCommandEvent_x                                        0xF2302C
#define __NetStatusToggle_x                                        0xF304D9
#define __PCNames_x                                                0xF31924
#define __RangeAttackReady_x                                       0xF3164C
#define __RMouseHeldTime_x                                         0xF30578
#define __RunWalkState_x                                           0xF30440
#define __ScreenMode_x                                             0xE6ED80
#define __ScreenX_x                                                0xF303F4
#define __ScreenY_x                                                0xF303F0
#define __ScreenXMax_x                                             0xF303F8
#define __ScreenYMax_x                                             0xF303FC
#define __ServerHost_x                                             0xF209F4
#define __ServerName_x                                             0xF66BE0
#define __ShiftKeyDown_x                                           0xF30AD0
#define __ShowNames_x                                              0xF31804
#define __Socials_x                                                0xF66CE0
#define __SubscriptionType_x                                       0x10F8A14
#define __TargetAggroHolder_x                                      0x10CA3EC
#define __ZoneType_x                                               0xF308D4
#define __GroupAggro_x                                             0x10CA42C
#define __LoginName_x                                              0xFB8B7C
#define __Inviter_x                                                0xFB49D8
#define __AttackOnAssist_x                                         0xF317C0
#define __UseTellWindows_x                                         0xF31ABC

//// 
//Section 1: Vital Offsets 
//// 
#define instCRaid_x                                                0xF23050
#define instEQZoneInfo_x                                           0xF306CC
#define instKeypressHandler_x                                      0xFB51D8
#define pinstActiveBanker_x                                        0xF23008
#define pinstActiveCorpse_x                                        0xF2300C
#define pinstActiveGMaster_x                                       0xF23010
#define pinstActiveMerchant_x                                      0xF23004
#define pinstAggroInfo_x                                           0xD2C3EC
#define pinstAltAdvManager_x                                       0xE6FEE8
#define pinstAuraMgr_x                                             0xD3CBF0
#define pinstBandageTarget_x                                       0xF22FF0
#define pinstCamActor_x                                            0xE6F7B8
#define pinstCDBStr_x                                              0xE6ED14
#define pinstCDisplay_x                                            0xF23018
#define pinstCEverQuest_x                                          0x10C2418
#define pinstEverQuestInfo_x                                       0xF303E8
#define pinstCharData_x                                            0xF22FD4
#define pinstCharSpawn_x                                           0xF22FFC
#define pinstControlledMissile_x                                   0xF22FD0
#define pinstControlledPlayer_x                                    0xF22FFC
#define pinstCSidlManager_x                                        0x1730E78
#define pinstCXWndManager_x                                        0x1730E70
#define instDynamicZone_x                                          0xF30288
#define pinstDZMember_x                                            0xF30398
#define pinstDZTimerInfo_x                                         0xF3039C
#define pinstEQItemList_x                                          0xF1F478
#define pinstEQObjectList_x                                        0xF204A4
#define instEQMisc_x                                               0xC037A8
#define pinstEQSoundManager_x                                      0xE704B8
#define instExpeditionLeader_x                                     0xF302D2
#define instExpeditionName_x                                       0xF30312
#define pinstGroup_x                                               0xF20AA6
#define pinstImeManager_x                                          0x1730E7C
#define pinstLocalPlayer_x                                         0xF22FE8
#define pinstMercenaryData_x                                       0xFB5840
#define pinstMercenaryStats_x                                      0x10CA538
#define pinstMercAltAbilities_x                                    0xE70270
#define pinstModelPlayer_x                                         0xF23014
#define pinstPCData_x                                              0xF22FD4
#define pinstSkillMgr_x                                            0xFB6838
#define pinstSpawnManager_x                                        0xFB6090
#define pinstSpellManager_x                                        0xFB6980
#define pinstSpellSets_x                                           0xFAD684
#define pinstStringTable_x                                         0xF22F7C
#define pinstSwitchManager_x                                       0xF20638
#define pinstTarget_x                                              0xF23000
#define pinstTargetObject_x                                        0xF22FD8
#define pinstTargetSwitch_x                                        0xF22FDC
#define pinstTaskMember_x                                          0xE6EC60
#define pinstTrackTarget_x                                         0xF22FF4
#define pinstTradeTarget_x                                         0xF22FE4
#define instTributeActive_x                                        0xC037CD
#define pinstViewActor_x                                           0xE6F7B4
#define pinstWorldData_x                                           0xF22FB8
#define pinstZoneAddr_x                                            0xF3096C
#define pinstPlayerPath_x                                          0xFB60F0
#define pinstTargetIndicator_x                                     0xFB6B18
#define pinstCTargetManager_x                                      0xFB6B78
#define EQObject_Top_x                                             0xF22FC8
#define pinstRealEstateItems_x                                     0xFB66A4
 
//// 
//Section 2:  UI Related Offsets 
//// 
#define pinstCRealEstateItemsWnd_x                                 0xE6F6D0
#define pinstCAchievementsWnd_x                                    0xE6F72C
#define pinstCTextOverlay_x                                        0xD2C980
#define pinstCAudioTriggersWindow_x                                0xD2C768
#define pinstCCharacterSelect_x                                    0xE6F63C
#define pinstCFacePick_x                                           0xE6F3EC
#define pinstCFindItemWnd_x                                        0xE6F6CC
#define pinstCNoteWnd_x                                            0xE6F5F4
#define pinstCBookWnd_x                                            0xE6F5FC
#define pinstCPetInfoWnd_x                                         0xE6F600
#define pinstCTrainWnd_x                                           0xE6F604
#define pinstCSkillsWnd_x                                          0xE6F608
#define pinstCSkillsSelectWnd_x                                    0xE6F60C
#define pinstCCombatSkillsSelectWnd_x                              0xE6F610
#define pinstCFriendsWnd_x                                         0xE6F614
#define pinstCAuraWnd_x                                            0xE6F618
#define pinstCRespawnWnd_x                                         0xE6F61C
#define pinstCBandolierWnd_x                                       0xE6F620
#define pinstCPotionBeltWnd_x                                      0xE6F624
#define pinstCAAWnd_x                                              0xE6F628
#define pinstCAlarmWnd_x                                           0xE6F634
#define pinstCGroupSearchFiltersWnd_x                              0xE6F62C
#define pinstCLoadskinWnd_x                                        0xE6F630
#define pinstCLargeDialogWnd_x                                     0x10C9498
#define pinstCMusicPlayerWnd_x                                     0xE6F638
#define pinstCMailWnd_x                                            0xE6F640
#define pinstCMailCompositionWnd_x                                 0xE6F644
#define pinstCMailAddressBookWnd_x                                 0xE6F648
#define pinstCRaidWnd_x                                            0xE6F650
#define pinstCRaidOptionsWnd_x                                     0xE6F654
#define pinstCBreathWnd_x                                          0xE6F658
#define pinstCMapViewWnd_x                                         0xE6F65C
#define pinstCMapToolbarWnd_x                                      0xE6F660
#define pinstCEditLabelWnd_x                                       0xE6F664
#define pinstCTargetWnd_x                                          0xE6F668
#define pinstCColorPickerWnd_x                                     0xE6F66C
#define pinstCPlayerWnd_x                                          0xE6F670
#define pinstCOptionsWnd_x                                         0xE6F674
#define pinstCBuffWindowNORMAL_x                                   0xE6F678
#define pinstCBuffWindowSHORT_x                                    0xE6F67C
#define pinstCharacterCreation_x                                   0xE6F680
#define pinstCCursorAttachment_x                                   0xE6F684
#define pinstCCastingWnd_x                                         0xE6F688
#define pinstCCastSpellWnd_x                                       0xE6F68C
#define pinstCSpellBookWnd_x                                       0xE6F690
#define pinstCInventoryWnd_x                                       0xE6F694
#define pinstCBankWnd_x                                            0xE6F69C
#define pinstCQuantityWnd_x                                        0xE6F6A0
#define pinstCLootWnd_x                                            0xE6F6A4
#define pinstCActionsWnd_x                                         0xE6F6A8
#define pinstCCombatAbilityWnd_x                                   0xE6F6B0
#define pinstCMerchantWnd_x                                        0xE6F6B4
#define pinstCTradeWnd_x                                           0xE6F6BC
#define pinstCSelectorWnd_x                                        0xE6F6C0
#define pinstCBazaarWnd_x                                          0xE6F6C4
#define pinstCBazaarSearchWnd_x                                    0xE6F6C8
#define pinstCGiveWnd_x                                            0xE6F6E8
#define pinstCTrackingWnd_x                                        0xE6F6F0
#define pinstCInspectWnd_x                                         0xE6F6F4
#define pinstCSocialEditWnd_x                                      0xE6F6F8
#define pinstCFeedbackWnd_x                                        0xE6F6FC
#define pinstCBugReportWnd_x                                       0xE6F700
#define pinstCVideoModesWnd_x                                      0xE6F704
#define pinstCTextEntryWnd_x                                       0xE6F70C
#define pinstCFileSelectionWnd_x                                   0xE6F710
#define pinstCCompassWnd_x                                         0xE6F714
#define pinstCPlayerNotesWnd_x                                     0xE6F718
#define pinstCGemsGameWnd_x                                        0xE6F71C
#define pinstCTimeLeftWnd_x                                        0xE6F738
#define pinstCPetitionQWnd_x                                       0xE6F73C
#define pinstCStoryWnd_x                                           0xE6F740
#define pinstCJournalTextWnd_x                                     0xE6F744
#define pinstCJournalCatWnd_x                                      0xE6F748
#define pinstCBodyTintWnd_x                                        0xE6F74C
#define pinstCServerListWnd_x                                      0xE6F750
#define pinstCAvaZoneWnd_x                                         0xE6F75C
#define pinstCBlockedBuffWnd_x                                     0xE6F760
#define pinstCBlockedPetBuffWnd_x                                  0xE6F764
#define pinstCInvSlotMgr_x                                         0xE6F7AC
#define pinstCContainerMgr_x                                       0xE6F7B0
#define pinstCAdventureLeaderboardWnd_x                            0x10C2DC8
#define pinstCAdventureRequestWnd_x                                0x10C2E40
#define pinstCAltStorageWnd_x                                      0x10C3120
#define pinstCAdventureStatsWnd_x                                  0x10C2EB8
#define pinstCBarterMerchantWnd_x                                  0x10C3E68
#define pinstCBarterSearchWnd_x                                    0x10C3EE0
#define pinstCBarterWnd_x                                          0x10C3F58
#define pinstCChatWindowManager_x                                  0x10C4768
#define pinstCDynamicZoneWnd_x                                     0x10C4C98
#define pinstCEQMainWnd_x                                          0x10C4E60
#define pinstCFellowshipWnd_x                                      0x10C4C2C
#define pinstCFindLocationWnd_x                                    0x10C5130
#define pinstCGroupSearchWnd_x                                     0x10C5400
#define pinstCGroupWnd_x                                           0x10C5478
#define pinstCGuildBankWnd_x                                       0x10C54F0
#define pinstCGuildMgmtWnd_x                                       0x10C75E0
#define pinstCHotButtonWnd_x                                       0x10C76DC
#define pinstCHotButtonWnd1_x                                      0x10C76DC
#define pinstCHotButtonWnd2_x                                      0x10C76E0
#define pinstCHotButtonWnd3_x                                      0x10C76E4
#define pinstCHotButtonWnd4_x                                      0x10C7708
#define pinstCItemDisplayManager_x                                 0x10C7B60
#define pinstCItemExpTransferWnd_x                                 0x10C7C5C
#define pinstCLFGuildWnd_x                                         0x10C7F38
#define pinstCMIZoneSelectWnd_x                                    0x10C8518
#define pinstCConfirmationDialog_x                                 0x10C8B30
#define pinstCPopupWndManager_x                                    0x10C8B30
#define pinstCProgressionSelectionWnd_x                            0x10C8C20
#define pinstCPvPStatsWnd_x                                        0x10C8D10
#define pinstCTaskWnd_x                                            0x10CA818
#define pinstCTaskSomething_x                                      0xD3D808
#define pinstCTaskTemplateSelectWnd_x                              0x10CA7A0
#define pinstCTipWndOFDAY_x                                        0x10CA980
#define pinstCTipWndCONTEXT_x                                      0x10CA984
#define pinstCTitleWnd_x                                           0x10CAA00
#define pinstCContextMenuManager_x                                 0x1730D00
#define pinstCVoiceMacroWnd_x                                      0xFB6F50
#define pinstCHtmlWnd_x                                            0xFB7040
#define pinstItemIconCache_x                                       0x10C4E08
#define pinstCTradeskillWnd_x                                      0x10CAB00
#define pinstCAdvancedLootWnd_x                                    0xE6F7A0
#define pinstRewardSelectionWnd_x                                  0x10C9238
#define pinstEQSuiteTextureLoader_x                                0xD1AA68

//// 
// Section 3: Miscellaneous Offsets 
////
#define __CastRay_x                                                0x54ACA0
#define __CastRay2_x                                               0x54A5D0
#define __HeadingDiff_x                                            0x933E50
#define __FixHeading_x                                             0x933DE0
#define __get_bearing_x                                            0x53A460
#define __ConvertItemTags_x                                        0x537F30
#define __ExecuteCmd_x                                             0x522870
#define __EQGetTime_x                                              0x8A35E0
#define __get_melee_range_x                                        0x529510
#define __GetGaugeValueFromEQ_x                                    0x7B3740
#define __GetLabelFromEQ_x                                         0x7B4B10
#define __ToggleKeyRingItem_x                                      0x48F930
#define __GetXTargetType_x                                         0x935AC0
#define __LoadFrontEnd_x                                           0x640B00
#define __NewUIINI_x                                               0x7B3110
#define __ProcessGameEvents_x                                      0x584820
#define __ProcessMouseEvent_x                                      0x583FC0
#define CrashDetected_x                                            0x6424C0
#define wwsCrashReportCheckForUploader_x                           0x801620
#define __AppCrashWrapper_x                                        0xC67AC4
#define __pCrashHandler_x                                          0x10D79E0
#define __CrashHandler_x                                           0x8010B0
#define wwsCrashReportPlatformLaunchUploader_x                     0x803850
#define DrawNetStatus_x                                            0x5BAED0
#define Util__FastTime_x                                           0x8A3280
#define Expansion_HoT_x                                            0xF317B0
#define __HelpPath_x                                               0xFB4F60
#define __STMLToText_x                                             0x8E9A20
#define __GetAnimationCache_x                                      0x6B0390

//// 
// Section 4: Function Offsets 
//// 
// CAdvancedLootWnd
#define CAdvancedLootWnd__CAdvancedLootWnd_x                       0x423390
#define CAdvancedLootWnd__DoAdvLootAction_x                        0x41DFF0
#define CAdvancedLootWnd__DoSharedAdvLootAction_x                  0x41DDC0

// AltAdvManager 
#define AltAdvManager__GetCalculatedTimer_x                        0x4D8200
#define AltAdvManager__IsAbilityReady_x                            0x4D8270
#define AltAdvManager__GetAAById_x                                 0x4D8760
#define AltAdvManager__CanTrainAbility_x                           0x4D9390
#define AltAdvManager__CanSeeAbility_x                             0x4D8F50

// CharacterZoneClient
#define CharacterZoneClient__CharacterZoneClient_x                 0x46AD40
#define CharacterZoneClient__HasSkill_x                            0x466C50
#define CharacterZoneClient__MakeMeVisible_x                       0x46C8B0
#define CharacterZoneClient__IsStackBlocked_x                      0x44BDC0
#define CharacterZoneClient__CanUseMemorizedSpellSlot_x            0x44B2E0
#define CharacterZoneClient__GetItemCountWorn_x                    0x4651B0
#define CharacterZoneClient__GetItemCountInInventory_x             0x465290
#define CharacterZoneClient__GetCursorItemCount_x                  0x465370
#define CharacterZoneClient__FindAffectSlot_x                      0x451470
#define CharacterZoneClient__BardCastBard_x                        0x44AA50
#define CharacterZoneClient__GetMaxEffects_x                       0x4495E0
#define CharacterZoneClient__GetEffect_x                           0x44B510
#define CharacterZoneClient__GetOpenEffectSlot_x                   0x4512D0
#define CharacterZoneClient__GetFirstEffectSlot_x                  0x449930
#define CharacterZoneClient__GetLastEffectSlot_x                   0x44FE20

// CBankWnd 
#define CBankWnd__GetNumBankSlots_x                                0x670470

// CBazaarSearchWnd 
#define CBazaarSearchWnd__HandleBazaarMsg_x                        0x67E2F0

// CButtonWnd 
#define CButtonWnd__SetCheck_x                                     0x599950
#define CButtonWnd__CButtonWnd_x                                   0x8DDCF0

// CChatManager 
#define CChatManager__GetRGBAFromIndex_x                           0x699480
#define CChatManager__InitContextMenu_x                            0x69A5D0
#define CChatManager__FreeChatWindow_x                             0x698F30
#define CChatManager__GetLockedActiveChatWindow_x                  0x6A2780
#define CChatManager__SetLockedActiveChatWindow_x                  0x699450

// CContextMenu 
#define CContextMenu__CContextMenu_x                               0x8E7D80
#define CContextMenu__dCContextMenu_x                              0x8E7F90
#define CContextMenu__AddMenuItem_x                                0x8E84A0
#define CContextMenu__RemoveMenuItem_x                             0x8E8100
#define CContextMenu__RemoveAllMenuItems_x                         0x8E8120
#define CContextMenu__CheckMenuItem_x                              0x8E81A0
#define CContextMenu__SetMenuItem_x                                0x8E8030

//CContextMenuManager
#define CContextMenuManager__AddMenu_x                             0x8D4260
#define CContextMenuManager__RemoveMenu_x                          0x8D45F0
#define CContextMenuManager__PopupMenu_x                           0x8D4A40
#define CContextMenuManager__Flush_x                               0x8D42D0
#define CContextMenuManager__GetMenu_x                             0x4194B0

// CChatService
#define CChatService__GetNumberOfFriends_x                         0x887760
#define CChatService__GetFriendName_x                              0x887770

// CChatWindow 
#define CChatWindow__CChatWindow_x                                 0x6A0250
#define CChatWindow__Clear_x                                       0x69FB30
#define CChatWindow__WndNotification_x                             0x6A0B60

// CComboWnd 
#define CComboWnd__DeleteAll_x                                     0x8D80B0
#define CComboWnd__Draw_x                                          0x8D82C0
#define CComboWnd__GetCurChoice_x                                  0x8D7E60
#define CComboWnd__GetListRect_x                                   0x8D8560
#define CComboWnd__GetTextRect_x                                   0x8D8120
#define CComboWnd__InsertChoice_x                                  0x8D86C0
#define CComboWnd__SetColors_x                                     0x8D7DF0
#define CComboWnd__SetChoice_x                                     0x8D7E20
#define CComboWnd__GetItemCount_x                                  0x8D7E50
#define CComboWnd__GetCurChoiceText_x                              0x8D7EA0


// CContainerWnd 
#define CContainerWnd__HandleCombine_x                             0x6AA510
#define CContainerWnd__vftable_x                                   0xAED9F8

// CDisplay 
#define CDisplay__CleanGameUI_x                                    0x4C6000
#define CDisplay__GetClickedActor_x                                0x4BE8C0
#define CDisplay__GetUserDefinedColor_x                            0x4BD4A0
#define CDisplay__GetWorldFilePath_x                               0x4BC8F0
#define CDisplay__is3dON_x                                         0x4BBD80
#define CDisplay__ReloadUI_x                                       0x4CFDC0
#define CDisplay__WriteTextHD2_x                                   0x4C2390
#define CDisplay__TrueDistance_x                                   0x4BF4F0

// CEditBaseWnd 
#define CEditBaseWnd__SetSel_x                                     0x8FF050

// CEditWnd 
#define CEditWnd__DrawCaret_x                                      0x8EC2D0
#define CEditWnd__GetCharIndexPt_x                                 0x8ED410
#define CEditWnd__GetDisplayString_x                               0x8EC480
#define CEditWnd__GetHorzOffset_x                                  0x8EC7C0
#define CEditWnd__GetLineForPrintableChar_x                        0x8ECEE0
#define CEditWnd__GetSelStartPt_x                                  0x8ED6C0
#define CEditWnd__GetSTMLSafeText_x                                0x8EC950
#define CEditWnd__PointFromPrintableChar_x                         0x8ECFD0
#define CEditWnd__SelectableCharFromPoint_x                        0x8ED150
#define CEditWnd__SetEditable_x                                    0x8EC920

// CEverQuest 
#define CEverQuest__DoPercentConvert_x                             0x56AF60
#define CEverQuest__ClickedPlayer_x                                0x564B30
#define CEverQuest__CreateTargetIndicator_x                        0x565A30
#define CEverQuest__DeleteTargetIndicator_x                        0x565AB0
#define CEverQuest__DoTellWindow_x                                 0x475FD0
#define CEverQuest__OutputTextToLog_x                              0x475C30
#define CEverQuest__DropHeldItemOnGround_x                         0x57B4D0
#define CEverQuest__dsp_chat_x                                     0x476310
#define CEverQuest__trimName_x                                     0x561440
#define CEverQuest__Emote_x                                        0x5797E0
#define CEverQuest__EnterZone_x                                    0x5748C0
#define CEverQuest__GetBodyTypeDesc_x                              0x55EE80
#define CEverQuest__GetClassDesc_x                                 0x568370
#define CEverQuest__GetClassThreeLetterCode_x                      0x568970
#define CEverQuest__GetDeityDesc_x                                 0x55F760
#define CEverQuest__GetLangDesc_x                                  0x55F210
#define CEverQuest__GetRaceDesc_x                                  0x568B30
#define CEverQuest__InterpretCmd_x                                 0x56C6E0
#define CEverQuest__LeftClickedOnPlayer_x                          0x581030
#define CEverQuest__LMouseUp_x                                     0x582FE0
#define CEverQuest__RightClickedOnPlayer_x                         0x581B40
#define CEverQuest__RMouseUp_x                                     0x582A40
#define CEverQuest__SetGameState_x                                 0x565400
#define CEverQuest__UPCNotificationFlush_x                         0x55D810
#define CEverQuest__IssuePetCommand_x                              0x56D0C0

// CGaugeWnd 
#define CGaugeWnd__CalcFillRect_x                                  0x6BB2F0
#define CGaugeWnd__CalcLinesFillRect_x                             0x6BB350
#define CGaugeWnd__Draw_x                                          0x6BB780

// CGuild
#define CGuild__FindMemberByName_x                                 0x43AF40
#define CGuild__GetGuildName_x                                     0x43B410
#define CGuild__GetGuildIndex_x                                    0x43C620

// CHotButtonWnd 
#define CHotButtonWnd__DoHotButton_x                               0x6D7810

// CInvSlotMgr 
#define CInvSlotMgr__FindInvSlot_x                                 0x6E4BD0
#define CInvSlotMgr__MoveItem_x                                    0x6E5390
#define CInvSlotMgr__SelectSlot_x                                  0x6E44C0

// CInvSLot
#define CInvSlot__HandleRButtonUp_x                                0x6E3920
#define CInvSlot__SliderComplete_x                                 0x6E1140
#define CInvSlot__GetItemBase_x                                    0x6E05C0
#define CInvSlot__UpdateItem_x                                     0x6E0B40

// CInvSlotWnd
#define CInvSlotWnd__DrawTooltip_x                                 0x6E67F0
#define CInvSlotWnd__CInvSlotWnd_x                                 0x6E7920

// CItemDisplayWnd 
#define CItemDisplayWnd__SetSpell_x                                0x7AF550
#define CItemDisplayWnd__UpdateStrings_x                           0x6E8A40
#define CItemDisplayWnd__InsertAugmentRequest_x                    0x6F5320
#define CItemDisplayWnd__RemoveAugmentRequest_x                    0x6F5850
#define CItemDisplayWnd__SetItem_x                                 0x6FB1A0

//CWndDisplayManager
#define CWndDisplayManager__FindWindowA_x                          0x7E6660

//CItemDisplayManager
#define CItemDisplayManager__CreateWindowInstance_x                0x6EF020

// CLabel 
#define CLabel__Draw_x                                             0x702010

// CListWnd
#define CListWnd__CListWnd_x                                       0x8C3C30
#define CListWnd__dCListWnd_x                                      0x8C4ED0
#define CListWnd__AddColumn_x                                      0x8C4E70
#define CListWnd__AddColumn1_x                                     0x8C4510
#define CListWnd__AddLine_x                                        0x8C3EF0
#define CListWnd__AddString_x                                      0x8C4110
#define CListWnd__CalculateFirstVisibleLine_x                      0x8C0D50
#define CListWnd__CalculateVSBRange_x                              0x8C30F0
#define CListWnd__ClearSel_x                                       0x8C0070
#define CListWnd__ClearAllSel_x                                    0x8C0020
#define CListWnd__CloseAndUpdateEditWindow_x                       0x8C3AC0
#define CListWnd__Compare_x                                        0x8C1A40
#define CListWnd__Draw_x                                           0x8C2D00
#define CListWnd__DrawColumnSeparators_x                           0x8C2B90
#define CListWnd__DrawHeader_x                                     0x8C0330
#define CListWnd__DrawItem_x                                       0x8C20D0
#define CListWnd__DrawLine_x                                       0x8C2830
#define CListWnd__DrawSeparator_x                                  0x8C2C30
#define CListWnd__EnableLine_x                                     0x8C0250
#define CListWnd__EnsureVisible_x                                  0x8C0EB0
#define CListWnd__ExtendSel_x                                      0x8C1FE0
#define CListWnd__GetColumnMinWidth_x                              0x8BF960
#define CListWnd__GetColumnWidth_x                                 0x8BF890
#define CListWnd__GetCurSel_x                                      0x8BF050
#define CListWnd__GetItemAtPoint_x                                 0x8C1140
#define CListWnd__GetItemAtPoint1_x                                0x8C11B0
#define CListWnd__GetItemData_x                                    0x8BF3A0
#define CListWnd__GetItemHeight_x                                  0x8C09B0
#define CListWnd__GetItemIcon_x                                    0x8BF590
#define CListWnd__GetItemRect_x                                    0x8C0F80
#define CListWnd__GetItemText_x                                    0x8BF440
#define CListWnd__GetSelList_x                                     0x8C4400
#define CListWnd__GetSeparatorRect_x                               0x8C1960
#define CListWnd__InsertLine_x                                     0x8C42F0
#define CListWnd__RemoveLine_x                                     0x8C4300
#define CListWnd__SetColors_x                                      0x8BF1A0
#define CListWnd__SetColumnJustification_x                         0x8BFBF0
#define CListWnd__SetColumnWidth_x                                 0x8BF910
#define CListWnd__SetCurSel_x                                      0x8BF090
#define CListWnd__SetItemColor_x                                   0x8C37B0
#define CListWnd__SetItemData_x                                    0x8C00D0
#define CListWnd__SetItemText_x                                    0x8C35F0
#define CListWnd__ShiftColumnSeparator_x                           0x8C3480
#define CListWnd__Sort_x                                           0x8C4FD0
#define CListWnd__ToggleSel_x                                      0x8BFFA0

// CMapViewWnd 
#define CMapViewWnd__CMapViewWnd_x                                 0x71B220

// CMerchantWnd 
#define CMerchantWnd__DisplayBuyOrSellPrice_x                      0x740730
#define CMerchantWnd__RequestBuyItem_x                             0x748220
#define CMerchantWnd__RequestSellItem_x                            0x74A830
#define CMerchantWnd__SelectRecoverySlot_x                         0x73D9E0
#define CMerchantWnd__SelectBuySellSlot_x                          0x73B980
#define CMerchantWnd__ActualSelect_x                               0x7424E0

// CPacketScrambler
#define CPacketScrambler__ntoh_x                                   0x855B90
#define CPacketScrambler__hton_x                                   0x855BA0

// CSidlManager 
#define CSidlManager__FindScreenPieceTemplate_x                    0x8E2CA0
#define CSidlManager__FindScreenPieceTemplate1_x                   0x8E2D80
#define CSidlManager__CreateLabel_x                                0x7A4520
#define CSidlManager__CreateXWndFromTemplate_x                     0x8E1230
#define CSidlManager__CreateXWndFromTemplate1_x                    0x8E3060

// CSidlScreenWnd 
#define CSidlScreenWnd__CalculateHSBRange_x                        0x8DA110
#define CSidlScreenWnd__CalculateVSBRange_x                        0x8DA000
#define CSidlScreenWnd__ConvertToRes_x                             0x8FEF30
#define CSidlScreenWnd__CreateChildrenFromSidl_x                   0x8DAA30
#define CSidlScreenWnd__CSidlScreenWnd1_x                          0x8DBC20
#define CSidlScreenWnd__CSidlScreenWnd2_x                          0x8DBCD0
#define CSidlScreenWnd__dCSidlScreenWnd_x                          0x8DB190
#define CSidlScreenWnd__DrawSidlPiece_x                            0x8DA3E0
#define CSidlScreenWnd__EnableIniStorage_x                         0x8D9B30
#define CSidlScreenWnd__GetSidlPiece_x                             0x8DA5D0
#define CSidlScreenWnd__Init1_x                                    0x8DB8F0
#define CSidlScreenWnd__LoadIniInfo_x                              0x8DAAF0
#define CSidlScreenWnd__LoadIniListWnd_x                           0x8D9CF0
#define CSidlScreenWnd__LoadSidlScreen_x                           0x8DB3D0
#define CSidlScreenWnd__StoreIniInfo_x                             0x8D9790
#define CSidlScreenWnd__StoreIniVis_x                              0x8D9AE0
#define CSidlScreenWnd__WndNotification_x                          0x8DB670
#define CSidlScreenWnd__GetChildItem_x                             0x8D9C00
#define CSidlScreenWnd__HandleLButtonUp_x                          0x8CD410
#define CSidlScreenWnd__m_layoutCopy_x                             0x1730E20

// CSkillMgr
#define CSkillMgr__IsAvailable_x                                   0x606240
#define CSkillMgr__GetSkillCap_x                                   0x606420
#define CSkillMgr__GetNameToken_x                                  0x605DF0

// CSliderWnd 
#define CSliderWnd__GetValue_x                                     0x8FE1E0
#define CSliderWnd__SetValue_x                                     0x8FE3B0
#define CSliderWnd__SetNumTicks_x                                  0x8FEB30

// CSpellBookWnd 
#define CSpellBookWnd__MemorizeSet_x                               0x7AA830

// CStmlWnd
#define CStmlWnd__AppendSTML_x                                     0x8F7D20
#define CStmlWnd__CalculateHSBRange_x                              0x8F0280
#define CStmlWnd__CalculateVSBRange_x                              0x8F01F0
#define CStmlWnd__CanBreakAtCharacter_x                            0x8F0400
#define CStmlWnd__FastForwardToEndOfTag_x                          0x8F0C00
#define CStmlWnd__ForceParseNow_x                                  0x8F86A0
#define CStmlWnd__GetNextTagPiece_x                                0x8F0B00
#define CStmlWnd__GetSTMLText_x                                    0x484160
#define CStmlWnd__GetVisibleText_x                                 0x8F1650
#define CStmlWnd__InitializeWindowVariables_x                      0x8F3C20
#define CStmlWnd__MakeStmlColorTag_x                               0x8EF4D0
#define CStmlWnd__MakeWndNotificationTag_x                         0x8EF530
#define CStmlWnd__SetSTMLText_x                                    0x8F5480
#define CStmlWnd__StripFirstSTMLLines_x                            0x8F79C0
#define CStmlWnd__UpdateHistoryString_x                            0x8F2590

// CTabWnd 
#define CTabWnd__Draw_x                                            0x8FBBB0
#define CTabWnd__DrawCurrentPage_x                                 0x8FB440
#define CTabWnd__DrawTab_x                                         0x8FB210
#define CTabWnd__GetCurrentPage_x                                  0x8FB7F0
#define CTabWnd__GetPageInnerRect_x                                0x8FAF00
#define CTabWnd__GetTabInnerRect_x                                 0x8FB100
#define CTabWnd__GetTabRect_x                                      0x8FAFD0
#define CTabWnd__InsertPage_x                                      0x8FBEA0
#define CTabWnd__SetPage_x                                         0x8FB820
#define CTabWnd__SetPageRect_x                                     0x8FBAF0
#define CTabWnd__UpdatePage_x                                      0x8FBE40
#define CTabWnd__GetPageFromTabIndex_x                             0x8FB160
#define CTabWnd__GetCurrentTabIndex_x                              0x8FAE90

// CPageWnd 
#define CPageWnd__GetTabText_x                                     0x6E7AD0
#define CPageWnd__SetTabText_x                                     0x908280

// CTextOverlay 
#define CTextOverlay__DisplayText_x                                0x436AB0

// CTextureFont
#define CTextureFont__DrawWrappedText_x                            0x8D7A60
#define CTextureFont__GetTextExtent_x                              0x8D77C0

//CWebManager
#define CWebManager__CreateHtmlWnd_x                               0x63ED90

// CXMLDataManager 
#define CXMLDataManager__GetXMLData_x                              0x9046C0

// CXMLSOMDocumentBase 
#define CXMLSOMDocumentBase__XMLRead_x                             0x8BD1B0

// CXStr 
// WARNING:  Be sure that none of these offsets are identical! 
// 
// Note:  dCXStr, CXStr1, & CXStr3 can be found in the 'BookWindow' constructor. 
#define CXStr__CXStr_x                                             0x5A7640
#define CXStr__CXStr1_x                                            0x7E2380
#define CXStr__CXStr3_x                                            0x8A04D0
#define CXStr__dCXStr_x                                            0x7B02F0
#define CXStr__operator_equal_x                                    0x8A0640
#define CXStr__operator_equal1_x                                   0x8A0690
#define CXStr__operator_plus_equal1_x                              0x8A1790

// CXWnd
//CTileLayoutWnd__SetOriginalAutoStretchInfo_x is here because its where I lookup the actual CXWnd Size.
#define CTileLayoutWnd__SetOriginalAutoStretchInfo_x               0x8D5560
#define CXWnd__BringToTop_x                                        0x8CC5E0
#define CXWnd__Center_x                                            0x8D2820
#define CXWnd__ClrFocus_x                                          0x8CC1C0
#define CXWnd__Destroy_x                                           0x8D3310
#define CXWnd__DoAllDrawing_x                                      0x8D2470
#define CXWnd__DrawChildren_x                                      0x8D2610
#define CXWnd__DrawColoredRect_x                                   0x8CC9D0
#define CXWnd__DrawTooltip_x                                       0x8CCC80
#define CXWnd__DrawTooltipAtPoint_x                                0x8D13E0
#define CXWnd__GetBorderFrame_x                                    0x8CD0E0
#define CXWnd__GetChildWndAt_x                                     0x8D31F0
#define CXWnd__GetClientClipRect_x                                 0x8CCE90
#define CXWnd__GetScreenClipRect_x                                 0x8D1AD0
#define CXWnd__GetScreenRect_x                                     0x8CD2B0
#define CXWnd__GetTooltipRect_x                                    0x8CCB10
#define CXWnd__GetWindowTextA_x                                    0x426E60
#define CXWnd__IsActive_x                                          0x8C7CB0
#define CXWnd__IsDescendantOf_x                                    0x8CD020
#define CXWnd__IsReallyVisible_x                                   0x8D0410
#define CXWnd__IsType_x                                            0x8D2890
#define CScreenPieceTemplate__IsType_x                             0x900DD0
#define CXWnd__Move_x                                              0x8CFB10
#define CXWnd__Move1_x                                             0x8D2120
#define CXWnd__ProcessTransition_x                                 0x8CC580
#define CXWnd__Refade_x                                            0x8CC350
#define CXWnd__Resize_x                                            0x8CD370
#define CXWnd__Right_x                                             0x8D18D0
#define CXWnd__SetFocus_x                                          0x8CEC70
#define CXWnd__SetFont_x                                           0x8CC210
#define CXWnd__SetKeyTooltip_x                                     0x8CD5E0
#define CXWnd__SetMouseOver_x                                      0x8D1E10
#define CXWnd__StartFade_x                                         0x8CC630
#define CXWnd__GetChildItem_x                                      0x8D1D50
#define CXWnd__SetParent_x                                         0x8D31A0

// CXWndManager 
#define CXWndManager__DrawCursor_x                                 0x8C9560
#define CXWndManager__DrawWindows_x                                0x8C91A0
#define CXWndManager__GetKeyboardFlags_x                           0x8C77E0
#define CXWndManager__HandleKeyboardMsg_x                          0x8C8140
#define CXWndManager__RemoveWnd_x                                  0x8C7E40

// CDBStr
#define CDBStr__GetString_x                                        0x4BA5C0

// EQ_Character 
#define EQ_Character__CastSpell_x                                  0x452530
#define EQ_Character__Cur_HP_x                                     0x466830
#define EQ_Character__Cur_Mana_x                                   0x45B810
#define EQ_Character__GetAACastingTimeModifier_x                   0x44B5F0
#define EQ_Character__GetFocusCastingTimeModifier_x                0x443540
#define EQ_Character__GetFocusRangeModifier_x                      0x443690
#define EQ_Character__GetHPRegen_x                                 0x46A3D0
#define EQ_Character__GetEnduranceRegen_x                          0x465E40
#define EQ_Character__GetManaRegen_x                               0x46A970
#define EQ_Character__Max_Endurance_x                              0x5CCA80
#define EQ_Character__Max_HP_x                                     0x45E390
#define EQ_Character__Max_Mana_x                                   0x5CC8B0
#define EQ_Character__doCombatAbility_x                            0x5C9F80
#define EQ_Character__UseSkill_x                                   0x472760
#define EQ_Character__GetConLevel_x                                0x5BF500
#define EQ_Character__IsExpansionFlag_x                            0x4B6770
#define EQ_Character__TotalEffect_x                                0x44F760
#define EQ_Character__GetPCSpellAffect_x                           0x44B980
#define EQ_Character__SpellDuration_x                              0x449980
#define EQ_Character__FindItemByRecord_x                           0x463E80
#define EQ_Character__GetAdjustedSkill_x                           0x466A00
#define EQ_Character__GetBaseSkill_x                               0x45E270

//ProfileManager
#define ProfileManager__GetCurrentProfile_x                        0x873860

//PlayerClient (to us known as EQPlayer)
#define PlayerClient__GetPcClient_x                                0x5D5180

//PcClient
#define PcClient__PcClient_x                                       0x5C1340

//CCharacterSelect
#define CCharacterSelect__SelectCharacter_x                        0x446AB0
#define CCharacterSelect__EnterWorld_x                             0x446280
#define CCharacterSelect__Quit_x                                   0x444E40

// EQ_Item (ItemClient)
#define EQ_Item__CanDrop_x                                         0x59F330
#define EQ_Item__CreateItemTagString_x                             0x853D30
#define EQ_Item__IsStackable_x                                     0x8491C0
#define EQ_Item__GetImageNum_x                                     0x84BD00
#define EQ_Item__CreateItemClient_x                                0x5A0460
#define EQ_Item__GetItemValue_x                                    0x852790
#define EQ_Item__ValueSellMerchant_x                               0x8557F0
#define EQ_Item__IsKeyRingItem_x                                   0x849D20
#define EQ_Item__CanGoInBag_x                                      0x59F4C0
#define EQ_Item__GetMaxItemCount_x                                 0x8490B0
#define EQ_Item__GetHeldItem_x                                     0x84AB20
#define EQ_Item__GetAugmentFitBySlot_x                             0x855610

// EQ_LoadingS 
#define EQ_LoadingS__SetProgressBar_x                              0x4DA7A0
#define EQ_LoadingS__Array_x                                       0xC0BC68

// EQ_PC 
#define EQ_PC__DestroyHeldItemOrMoney_x                            0x5CEFB0
#define EQ_PC__GetAlternateAbilityId_x                             0x85DB90
#define EQ_PC__GetCombatAbility_x                                  0x85DC20
#define EQ_PC__GetCombatAbilityTimer_x                             0x858840
#define EQ_PC__GetItemRecastTimer_x                                0x5C8770
#define EQ_PC__HasLoreItem_x                                       0x5C2B00
#define EQ_PC__AlertInventoryChanged_x                             0x5BF4B0
#define EQ_PC__GetPcZoneClient_x                                   0x5E9740
#define EQ_PC__RemoveMyAffect_x                                    0x5C7BA0
#define EQ_PC__GetKeyRingItems_x                                   0x858C60
#define EQ_PC__GetItemContainedRealEstateIds_x                     0x86B8C0
#define EQ_PC__GetNonArchivedOwnedRealEstates_x                    0x85FB30

// EQItemList 
#define EQItemList__EQItemList_x                                   0x5275C0
#define EQItemList__add_object_x                                   0x554770
#define EQItemList__add_item_x                                     0x527500
#define EQItemList__delete_item_x                                  0x5279B0
#define EQItemList__FreeItemList_x                                 0x5278B0

// EQMisc
#define EQMisc__GetActiveFavorCost_x                               0x4B9790

// EQPlayer 
#define EQPlayer__ChangeBoneStringSprite_x                         0x5D4E40
#define EQPlayer__dEQPlayer_x                                      0x5DD600
#define EQPlayer__DoAttack_x                                       0x5EEEF0
#define EQPlayer__EQPlayer_x                                       0x5DFD20
#define EQPlayer__SetNameSpriteState_x                             0x5DA430
#define EQPlayer__SetNameSpriteTint_x                              0x5D6340
#define PlayerBase__HasProperty_j_x                                0x932260
#define EQPlayer__IsTargetable_x                                   0x932500
#define EQPlayer__CanSee_x                                         0x932A60
#define PlayerZoneClient__ChangeHeight_x                           0x5EEDB0
#define EQPlayer__CanSee1_x                                        0x932B50
#define PlayerBase__GetVisibilityLineSegment_x                     0x9327F0

//PlayerZoneClient
#define PlayerZoneClient__GetLevel_x                               0x5E98F0
#define PlayerZoneClient__IsValidTeleport_x                        0x555030
#define PlayerZoneClient__LegalPlayerRace_x                        0x4D29B0


//EQPlayerManager
#define EQPlayerManager__GetSpawnByID_x                            0x5E3D70
#define EQPlayerManager__GetSpawnByName_x                          0x5E41D0
#define EQPlayerManager__GetPlayerFromPartialName_x                0x5E3DA0

// KeyPressHandler 
#define KeypressHandler__AttachAltKeyToEqCommand_x                 0x5A59D0
#define KeypressHandler__AttachKeyToEqCommand_x                    0x5A5A10
#define KeypressHandler__ClearCommandStateArray_x                  0x5A55C0
#define KeypressHandler__HandleKeyDown_x                           0x5A3EE0
#define KeypressHandler__HandleKeyUp_x                             0x5A4200
#define KeypressHandler__SaveKeymapping_x                          0x5A5690

// MapViewMap 
#define MapViewMap__Clear_x                                        0x713BB0
#define MapViewMap__SaveEx_x                                       0x716EE0
#define MapViewMap__SetZoom_x                                      0x713830

#define PlayerPointManager__GetAltCurrency_x                       0x871920

// StringTable 
#define StringTable__getString_x                                   0x86BBF0

// PcZoneClient
#define PcZoneClient__GetPcSkillLimit_x                            0x5CC510
#define PcZoneClient__RemovePetEffect_x                            0x5C8C40
#define PcZoneClient__HasAlternateAbility_x                        0x5C95E0
#define PcZoneClient__GetCurrentMod_x                              0x45D840
#define PcZoneClient__GetModCap_x                                  0x45AF70
#define PcZoneClient__CanEquipItem_x                               0x5C9AC0
#define PcZoneClient__GetItemByID_x                                0x5CE320
#define PcZoneClient__GetItemByItemClass_x                         0x5CEED0
#define PcZoneClient__RemoveBuffEffect_x                           0x5C7E50

//Doors
#define EQSwitch__UseSwitch_x                                      0x55C150

//IconCache
#define IconCache__GetIcon_x                                       0x6B09C0

//CContainerMgr
#define CContainerMgr__OpenContainer_x                             0x6A7890
#define CContainerMgr__CloseContainer_x                            0x6A80B0

//CQuantityWnd
#define CQuantityWnd__Open_x                                       0x7744E0

//CHelpWnd
#define CHelpWnd__SetFile_x                                        0x598370

//EQ_Spell
#define EQ_Spell__GetSpellLevelNeeded_x                            0x4F4C60
#define EQ_Spell__SpellAffects_x                                   0x4F1E40
#define EQ_Spell__SpellAffectBase_x                                0x4F1EB0
#define CharacterZoneClient__CalcAffectChange_x                    0x44B810
#define CharacterZoneClient__CalcAffectChangeGeneric_x             0x44AAB0
#define CLootWnd__LootAll_x                                        0x709470
#define CLootWnd__RequestLootSlot_x                                0x709C80
#define EQ_Spell__IsStackable_x                                    0x459000
#define EQ_Spell__GetSpellAffectBySlot_x                           0x449450
#define EQ_Spell__GetSpellAffectByIndex_x                          0x43EA90
#define EQ_Spell__IsSPAStacking_x                                  0x4F19B0
#define EQ_Spell__IsSPAIgnoredByStacking_x                         0x4F2A50
#define EQ_Spell__IsNoRemove_x                                     0x434AE0
#define EQ_Spell__IsDegeneratingLevelMod_x                         0x4F19D0

#define __IsResEffectSpell_x                                       0x4494D0

//EQ_Affect
#define EQ_Affect__GetAffectData_x                                 0x87D4E0

//CTargetWnd
#define CTargetWnd__GetBuffCaster_x                                0x7BC210
#define CTargetWnd__WndNotification_x                              0x7BBD00
#define CTargetWnd__RefreshTargetBuffs_x                           0x7BC3F0

//CTaskWnd
#define CTaskWnd__UpdateTaskTimers_x                               0x7C1350

//EqSoundManager
#define EqSoundManager__WavePlay_x                                 0x4ED510
#define EqSoundManager__PlayScriptMp3_x                            0x4EEFB0

//CCombatSkillsSelectWnd
#define CCombatSkillsSelectWnd__ShouldDisplayThisSkill_x           0x4A86F0

//CTextureAnimation
#define CTextureAnimation__SetCurCell_x                            0x8EB900
#define CSidlManager__FindAnimation1_x                             0x8E0C60

//CAltAbilityData
#define CAltAbilityData__GetMercCurrentRank_x                      0x4E1F50
#define CAltAbilityData__GetMercMaxRank_x                          0x4E1F00
#define CAltAbilityData__GetMaxRank_x                              0x4D68B0

//CTargetRing
#define CTargetRing__Cast_x                                        0x595FB0

//CharacterBase
#define CharacterBase__GetMemorizedSpell_x                         0x4592C0
#define CharacterBase__CreateItemGlobalIndex_x                     0x475180
#define CharacterBase__CreateItemIndex_x                           0x520BE0
#define CharacterBase__GetItemPossession_x                         0x4442E0
#define CharacterBase__GetItemByGlobalIndex_x                      0x881330
#define CharacterBase__GetEffectId_x                               0x459280

//CCastSpellWnd
#define CCastSpellWnd__ForgetMemorizedSpell_x                      0x6906C0
#define CCastSpellWnd__IsBardSongPlaying_x                         0x68FE20

//messages
#define msg_spell_worn_off_x                                       0x4FC760
#define msg_new_text_x                                             0x505EE0
#define msgTokenTextParam_x                                        0x5026E0

//SpellManager
#define SpellManager__vftable_x                                    0xAC9258
#define SpellManager__SpellManager_x                               0x47DDB0
#define Spellmanager__LoadTextSpells_x                             0x60D140

//CCollisionInfoTargetVisibility
#define CCollisionInfoTargetVisibility__CCollisionInfoTargetVisibility_x   0x9365E0

//ItemGlobalIndex
#define ItemGlobalIndex__IsKeyRingLocation_x                       0x487960
#define ItemGlobalIndex__ItemGlobalIndex_x                         0x43E9B0
#define ItemGlobalIndex__IsEquippedLocation_x                      0x4491A0
#define ItemGlobalIndex__IsValidIndex_x                            0x458B40

//ItemBaseContainer
#define ItemBaseContainer__ItemBaseContainer_x                     0x87F1A0
#define ItemBaseContainer__CreateItemGlobalIndex_x                 0x87E0E0

//CLargeDialogWnd
#define CLargeDialogWnd__Open_x                                    0x702260

//CCursorAttachment
#define CCursorAttachment__AttachToCursor_x                        0x6AC5F0
#define CCursorAttachment__Deactivate_x                            0x6ABB30

//CEQSuiteTextureLoader
#define CEQSuiteTextureLoader__GetDefaultUIPath_x                  0x8FD6E0
#define CEQSuiteTextureLoader__GetTexture_x                        0x8FD860

//ScreenWndManager (has the size of CSidlScreenWnd__CSidlScreenWnd1_x
//and thats the reason I added it here, we dont actually call it. - eqmule
#define ScreenWndManager__RegisterManaged_x                        0x604D70

//CFindItemWnd
#define CFindItemWnd__CFindItemWnd_x                               0x48C1E0

//IString
#define IString__Append_x                                          0x47EA30

//Camera
#define CDisplay__cameraType_x                                     0xE6F7C8
#define EverQuest__Cameras_x                                       0xF31AC8
