// SPDX-License-Identifier: GPL-2.0+
// [dwc_ddrphy_phyinit_main] Start of dwc_ddrphy_phyinit_main()
// [dwc_ddrphy_phyinit_sequence] Start of dwc_ddrphy_phyinit_sequence()
// [dwc_ddrphy_phyinit_initStruct] Start of dwc_ddrphy_phyinit_initStruct()
// [dwc_ddrphy_phyinit_initStruct] End of dwc_ddrphy_phyinit_initStruct()
// [dwc_ddrphy_phyinit_setDefault] Start of dwc_ddrphy_phyinit_setDefault()
// [dwc_ddrphy_phyinit_setDefault] End of dwc_ddrphy_phyinit_setDefault()

////##############################################################
//
//// dwc_ddrphy_phyinit_userCustom_overrideUserInput is a user-editable function.  User can edit this function according to their needs.
////
//// The purpose of dwc_ddrphy_phyinit_userCustom_overrideUserInput() is to override any
//// any field in Phyinit data structure set by dwc_ddrphy_phyinit_setDefault()
//// User should only override values in userInputBasic and userInputAdvanced.
//// IMPORTANT: in this function, user shall not override any values in the
//// messageblock directly on the data structue as the might be overwritten by
//// dwc_ddrphy_phyinit_calcMb().  Use dwc_ddrphy_phyinit_setMb() to set
//// messageblock parameters for override values to remain pervasive if
//// desired
//
////##############################################################

dwc_ddrphy_phyinit_userCustom_overrideUserInput();
//
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DramType' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DimmType' to 0x4
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'NumDbyte' to 0x2
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'NumActiveDbyteDfi0' to 0x2
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'NumAnib' to 0xa
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'NumRank_dfi0' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DramDataWidth[0]' to 0x10
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DramDataWidth[1]' to 0x10
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DramDataWidth[2]' to 0x10
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DramDataWidth[3]' to 0x10
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'NumPStates' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'Frequency[0]' to 0x640
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'PllBypass[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DfiFreqRatio[0]' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'Dfi1Exists' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'D4RxPreambleLength[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'D4TxPreambleLength[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'ExtCalResVal' to 0xf0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'Is2Ttiming[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'ODTImpedance[0]' to 0x78
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxImpedance[0]' to 0x3c
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'MemAlertEn' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'MtestPUImp' to 0xf0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DisDynAdrTri[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'PhyMstrTrainInterval[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'PhyMstrMaxReqToAck[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'PhyMstrCtrlMode[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'WDQSExt' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'CalInterval' to 0x9
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'CalOnce' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'RxEnBackOff' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TrainSequenceCtrl' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'SnpsUmctlOpt' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'SnpsUmctlF0RC5x[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewRiseDQ[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewFallDQ[0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewRiseAC' to 0x45
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewFallAC' to 0xa
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'IsHighVDD' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewRiseCK' to 0x52
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'TxSlewFallCK' to 0x12
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'DisablePmuEcc' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'EnableMAlertAsync' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'Apb32BitMode' to 0x1
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tDQS2DQ' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tDQSCK' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tCASL_override' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tCASL_add[0][0]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tCASL_add[0][1]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tCASL_add[0][2]' to 0x0
//// [dwc_ddrphy_phyinit_setUserInput] Setting PHYINIT field 'tCASL_add[0][3]' to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MsgMisc to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].Pstate to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].PllBypassEn to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].DRAMFreq to 0xc80
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].PhyVref to 0x40
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].DramType to 0x2
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].DisabledDbyte to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].EnabledDQs to 0x10
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].CsPresent to 0x1
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].CsPresentD0 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].CsPresentD1 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AddrMirror to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].PhyCfg to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].SequenceCtrl to 0x1
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].HdtCtrl to 0xff
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].PhyConfigOverride to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].DFIMRLMargin to 0x2
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR0 to 0x2150
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR1 to 0x1
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR2 to 0x28
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR3 to 0x400
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR4 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR5 to 0x400
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].MR6 to 0x104f
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].X16Present to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].CsSetupGDDec to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl0 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl1 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl2 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl3 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl4 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl5 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl6 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].AcsmOdtCtrl7 to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib0 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib1 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib2 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib3 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib4 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib5 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib6 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib7 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib8 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib9 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib10 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib11 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib12 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib13 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib14 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib15 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib16 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib17 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib18 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR0Nib19 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib0 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib1 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib2 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib3 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib4 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib5 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib6 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib7 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib8 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib9 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib10 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib11 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib12 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib13 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib14 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib15 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib16 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib17 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib18 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR1Nib19 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib0 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib1 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib2 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib3 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib4 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib5 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib6 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib7 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib8 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib9 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib10 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib11 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib12 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib13 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib14 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib15 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib16 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib17 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib18 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR2Nib19 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib0 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib1 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib2 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib3 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib4 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib5 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib6 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib7 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib8 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib9 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib10 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib11 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib12 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib13 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib14 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib15 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib16 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib17 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib18 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].VrefDqR3Nib19 to 0xf
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].ALT_CAS_L to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].ALT_WCAS_L to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].D4Misc to 0x1
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].ExtTrainOpt to 0x0
// [dwc_ddrphy_phyinit_setMb] Setting mb_DDR4U_1D[0].NVDIMM to 0x0
// [dwc_ddrphy_phyinit_userCustom_overrideUserInput] End of dwc_ddrphy_phyinit_userCustom_overrideUserInput()
//[dwc_ddrphy_phyinit_calcMb] Start of dwc_ddrphy_phyinit_calcMb()
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].DramType override to 0x2
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].Pstate override to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].DRAMFreq override to 0xc80
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].PllBypassEn override to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].EnabledDQs override to 0x10
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].PhyCfg override to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].DisabledDbyte override to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] mb_DDR4U_1D[0].X16Present override to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].DramType to 0x2
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].Pstate to 0x1
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].DRAMFreq to 0x856
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].PllBypassEn to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].EnabledDQs to 0x10
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].PhyCfg to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].DisabledDbyte to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[1].X16Present to 0x1
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].DramType to 0x2
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].Pstate to 0x2
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].DRAMFreq to 0x74a
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].PllBypassEn to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].EnabledDQs to 0x10
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].PhyCfg to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].DisabledDbyte to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[2].X16Present to 0x1
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].DramType to 0x2
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].Pstate to 0x3
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].DRAMFreq to 0x640
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].PllBypassEn to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].EnabledDQs to 0x10
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].PhyCfg to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].DisabledDbyte to 0x0
//// [dwc_ddrphy_phyinit_softSetMb] Setting mb_DDR4U_1D[3].X16Present to 0x1
////[dwc_ddrphy_phyinit_calcMb] TG_active[0] = 1
////[dwc_ddrphy_phyinit_calcMb] TG_active[1] = 0
////[dwc_ddrphy_phyinit_calcMb] TG_active[2] = 0
////[dwc_ddrphy_phyinit_calcMb] TG_active[3] = 0
////[dwc_ddrphy_phyinit_calcMb] tDIMM_CK [pstate=0][tg=0] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_DQ [pstate=0][tg=0] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] userInputSim.tCASL_add[pstate=0][tg=0] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_CK [pstate=0][tg=1] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_DQ [pstate=0][tg=1] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] userInputSim.tCASL_add[pstate=0][tg=1] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_CK [pstate=0][tg=2] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_DQ [pstate=0][tg=2] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] userInputSim.tCASL_add[pstate=0][tg=2] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_CK [pstate=0][tg=3] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] tDIMM_DQ [pstate=0][tg=3] = 0 ps
////[dwc_ddrphy_phyinit_calcMb] userInputSim.tCASL_add[pstate=0][tg=3] = 0 ps
//[dwc_ddrphy_phyinit_calcMb] End of dwc_ddrphy_phyinit_calcMb()
//// [phyinit_print_dat] // ####################################################
//// [phyinit_print_dat] //
//// [phyinit_print_dat] // Printing values in user input structure
//// [phyinit_print_dat] //
//// [phyinit_print_dat] // ####################################################
//// [phyinit_print_dat] pUserInputBasic->Frequency[0] = 1600
//// [phyinit_print_dat] pUserInputBasic->Frequency[1] = 1067
//// [phyinit_print_dat] pUserInputBasic->Frequency[2] = 933
//// [phyinit_print_dat] pUserInputBasic->Frequency[3] = 800
//// [phyinit_print_dat] pUserInputBasic->NumAnib = 10
//// [phyinit_print_dat] pUserInputBasic->DramType = 0
//// [phyinit_print_dat] pUserInputBasic->ARdPtrInitValOvr = 0
//// [phyinit_print_dat] pUserInputBasic->ARdPtrInitVal[0] = 3
//// [phyinit_print_dat] pUserInputBasic->ARdPtrInitVal[1] = 3
//// [phyinit_print_dat] pUserInputBasic->ARdPtrInitVal[2] = 3
//// [phyinit_print_dat] pUserInputBasic->ARdPtrInitVal[3] = 3
//// [phyinit_print_dat] pUserInputBasic->DfiFreqRatio[0] = 1
//// [phyinit_print_dat] pUserInputBasic->DfiFreqRatio[1] = 1
//// [phyinit_print_dat] pUserInputBasic->DfiFreqRatio[2] = 1
//// [phyinit_print_dat] pUserInputBasic->DfiFreqRatio[3] = 1
//// [phyinit_print_dat] pUserInputBasic->NumActiveDbyteDfi0 = 2
//// [phyinit_print_dat] pUserInputBasic->DisPtrInitClrTxTracking[0] = 0
//// [phyinit_print_dat] pUserInputBasic->DisPtrInitClrTxTracking[1] = 0
//// [phyinit_print_dat] pUserInputBasic->DisPtrInitClrTxTracking[2] = 0
//// [phyinit_print_dat] pUserInputBasic->DisPtrInitClrTxTracking[3] = 0
//// [phyinit_print_dat] pUserInputBasic->DramDataWidth[0] = 16
//// [phyinit_print_dat] pUserInputBasic->DramDataWidth[1] = 16
//// [phyinit_print_dat] pUserInputBasic->DramDataWidth[2] = 16
//// [phyinit_print_dat] pUserInputBasic->DramDataWidth[3] = 16
//// [phyinit_print_dat] pUserInputBasic->PllBypass[0] = 0
//// [phyinit_print_dat] pUserInputBasic->PllBypass[1] = 0
//// [phyinit_print_dat] pUserInputBasic->PllBypass[2] = 0
//// [phyinit_print_dat] pUserInputBasic->PllBypass[3] = 0
//// [phyinit_print_dat] pUserInputBasic->Dfi1Exists = 0
//// [phyinit_print_dat] pUserInputBasic->Train2D = 0
//// [phyinit_print_dat] pUserInputBasic->NumRank_dfi0 = 1
//// [phyinit_print_dat] pUserInputBasic->DimmType = 4
//// [phyinit_print_dat] pUserInputBasic->NumPStates = 1
//// [phyinit_print_dat] pUserInputBasic->NumDbyte = 2
//// [phyinit_print_dat] pUserInputAdvanced->DisablePmuEcc = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[0] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[1] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[2] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[3] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[4] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[5] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[6] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvEn[7] = 0
//// [phyinit_print_dat] pUserInputAdvanced->SnpsUmctlOpt = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl1[0] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl1[1] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl1[2] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl1[3] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallAC = 10
//// [phyinit_print_dat] pUserInputAdvanced->CalOnce = 0
//// [phyinit_print_dat] pUserInputAdvanced->ExtCalResVal = 240
//// [phyinit_print_dat] pUserInputAdvanced->D4TxPreambleLength[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->D4TxPreambleLength[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->D4TxPreambleLength[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->D4TxPreambleLength[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DramByteSwap[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DramByteSwap[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DramByteSwap[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DramByteSwap[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[2] = 11
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[3] = 11
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[4] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[5] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[6] = 0
//// [phyinit_print_dat] pUserInputAdvanced->NvAnibRcvSel[7] = 0
//// [phyinit_print_dat] pUserInputAdvanced->CalInterval = 9
//// [phyinit_print_dat] pUserInputAdvanced->IsHighVDD = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseAC = 69
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseCK = 82
//// [phyinit_print_dat] pUserInputAdvanced->RedundantCs_en = 0
//// [phyinit_print_dat] pUserInputAdvanced->TrainSequenceCtrl = 1
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrCtrlMode[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrCtrlMode[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrCtrlMode[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrCtrlMode[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallDQ[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallDQ[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallDQ[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallDQ[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->MtestPUImp = 240
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[0] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[1] = 3
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[2] = 1
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[3] = 3
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[4] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[5] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[6] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AnibRcvLaneSel[7] = 0
//// [phyinit_print_dat] pUserInputAdvanced->AlertRecoveryEnable = 0
//// [phyinit_print_dat] pUserInputAdvanced->DisDynAdrTri[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DisDynAdrTri[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DisDynAdrTri[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->DisDynAdrTri[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->VREGCtrl_LP2_PwrSavings_En = 0
//// [phyinit_print_dat] pUserInputAdvanced->SnpsUmctlF0RC5x[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->SnpsUmctlF0RC5x[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->SnpsUmctlF0RC5x[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->SnpsUmctlF0RC5x[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->D4RxPreambleLength[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->D4RxPreambleLength[1] = 1
//// [phyinit_print_dat] pUserInputAdvanced->D4RxPreambleLength[2] = 1
//// [phyinit_print_dat] pUserInputAdvanced->D4RxPreambleLength[3] = 1
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrMaxReqToAck[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrMaxReqToAck[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrMaxReqToAck[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrMaxReqToAck[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseDQ[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseDQ[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseDQ[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewRiseDQ[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxSlewFallCK = 18
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedance[0] = 60
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedance[1] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedance[2] = 25
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedance[3] = 25
//// [phyinit_print_dat] pUserInputAdvanced->en_16LogicalRanks_3DS = 0
//// [phyinit_print_dat] pUserInputAdvanced->RstRxTrkState = 0
//// [phyinit_print_dat] pUserInputAdvanced->Is2Ttiming[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->Is2Ttiming[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->Is2Ttiming[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->Is2Ttiming[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->MemAlertEn = 0
//// [phyinit_print_dat] pUserInputAdvanced->rtt_term_en = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrTrainInterval[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrTrainInterval[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrTrainInterval[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->PhyMstrTrainInterval[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->WDQSExt = 0
//// [phyinit_print_dat] pUserInputAdvanced->en_3DS = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl2[0] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl2[1] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl2[2] = 0
//// [phyinit_print_dat] pUserInputAdvanced->TxImpedanceCtrl2[3] = 0
//// [phyinit_print_dat] pUserInputAdvanced->ODTImpedance[0] = 120
//// [phyinit_print_dat] pUserInputAdvanced->ODTImpedance[1] = 60
//// [phyinit_print_dat] pUserInputAdvanced->ODTImpedance[2] = 60
//// [phyinit_print_dat] pUserInputAdvanced->ODTImpedance[3] = 60
//// [phyinit_print_dat] pUserInputAdvanced->EnableMAlertAsync = 0
//// [phyinit_print_dat] pUserInputAdvanced->Apb32BitMode = 1
//// [phyinit_print_dat] pUserInputAdvanced->Nibble_ECC = 15
//// [phyinit_print_dat] pUserInputAdvanced->RxEnBackOff = 1
//// [phyinit_print_dat] pUserInputAdvanced->ATxImpedance = 53247
//// [phyinit_print_dat] pUserInputSim->tDQS2DQ    = 0
//// [phyinit_print_dat] pUserInputSim->tDQSCK     = 0
//// [phyinit_print_dat] pUserInputSim->tSTAOFF[0] = 0
//// [phyinit_print_dat] pUserInputSim->tSTAOFF[1] = 0
//// [phyinit_print_dat] pUserInputSim->tSTAOFF[2] = 0
//// [phyinit_print_dat] pUserInputSim->tSTAOFF[3] = 0
//// [phyinit_print_dat] // ####################################################
//// [phyinit_print_dat] //
//// [phyinit_print_dat] // Printing values of 1D message block input/inout fields, PState=0
//// [phyinit_print_dat] //
//// [phyinit_print_dat] // ####################################################
//// [phyinit_print_dat] mb_DDR4U_1D[0].AdvTrainOpt = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MsgMisc = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].Pstate = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PllBypassEn = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DRAMFreq = 0xc80
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyVref = 0x40
//// [phyinit_print_dat] mb_DDR4U_1D[0].DramType = 0x2
//// [phyinit_print_dat] mb_DDR4U_1D[0].DisabledDbyte = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].EnabledDQs = 0x10
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresent = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresentD0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresentD1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AddrMirror = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyCfg = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].SequenceCtrl = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].HdtCtrl = 0xff
//// [phyinit_print_dat] mb_DDR4U_1D[0].Rx2D_CmdSpacing = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MREP_MIN_PULSE = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DWL_MIN_PULSE = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyConfigOverride = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DFIMRLMargin = 0x2
//// [phyinit_print_dat] mb_DDR4U_1D[0].DDR4_RXEN_OFFSET = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR0 = 0x2150
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR1 = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR2 = 0x28
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR3 = 0x400
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR5 = 0x400
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR6 = 0x104f
//// [phyinit_print_dat] mb_DDR4U_1D[0].X16Present = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsSetupGDDec = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK2 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK3 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK5 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK6 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK7 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl2 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl3 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl5 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl6 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl7 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].ALT_CAS_L = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].ALT_WCAS_L = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].D4Misc = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].ExtTrainOpt = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].NVDIMM = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AdvTrainOpt = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MsgMisc = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].Pstate = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PllBypassEn = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DRAMFreq = 0xc80
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyVref = 0x40
//// [phyinit_print_dat] mb_DDR4U_1D[0].DramType = 0x2
//// [phyinit_print_dat] mb_DDR4U_1D[0].DisabledDbyte = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].EnabledDQs = 0x10
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresent = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresentD0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsPresentD1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AddrMirror = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyCfg = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].SequenceCtrl = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].HdtCtrl = 0xff
//// [phyinit_print_dat] mb_DDR4U_1D[0].Rx2D_CmdSpacing = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MREP_MIN_PULSE = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DWL_MIN_PULSE = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].PhyConfigOverride = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].DFIMRLMargin = 0x2
//// [phyinit_print_dat] mb_DDR4U_1D[0].DDR4_RXEN_OFFSET = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR0 = 0x2150
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR1 = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR2 = 0x28
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR3 = 0x400
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR5 = 0x400
//// [phyinit_print_dat] mb_DDR4U_1D[0].MR6 = 0x104f
//// [phyinit_print_dat] mb_DDR4U_1D[0].X16Present = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].CsSetupGDDec = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK2 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK3 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK5 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK6 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].RTT_NOM_WR_PARK7 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl0 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl1 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl2 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl3 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl4 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl5 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl6 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].AcsmOdtCtrl7 = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR0Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR1Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR2Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib0 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib1 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib2 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib3 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib4 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib5 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib6 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib7 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib8 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib9 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib10 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib11 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib12 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib13 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib14 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib15 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib16 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib17 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib18 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].VrefDqR3Nib19 = 0xf
//// [phyinit_print_dat] mb_DDR4U_1D[0].ALT_CAS_L = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].ALT_WCAS_L = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].D4Misc = 0x1
//// [phyinit_print_dat] mb_DDR4U_1D[0].ExtTrainOpt = 0x0
//// [phyinit_print_dat] mb_DDR4U_1D[0].NVDIMM = 0x0

////##############################################################
////
//// Step (A) : Bring up VDD, VDDQ, and VAA
////
//// The power supplies can come up and stabilize in any order.
//// While the power supplies are coming up, all outputs will be unknown and
//// the values of the inputs are don't cares.
////
////##############################################################

dwc_ddrphy_phyinit_userCustom_A_bringupPower();

//[dwc_ddrphy_phyinit_userCustom_A_bringupPower] End of dwc_ddrphy_phyinit_userCustom_A_bringupPower()
//
//
////##############################################################
////
//// 4.3.2(B) Start Clocks and Reset the PHY
////
//// Following is one possbile sequence to reset the PHY. Other sequences are also possible.
//// See section 5.2.2 of the PUB for other possible reset sequences.
////
//// 1. Drive PwrOkIn to 0. Note: Reset, DfiClk, and APBCLK can be X.
//// 2. Start DfiClk and APBCLK
//// 3. Drive Reset to 1 and PRESETn_APB to 0.
////    Note: The combination of PwrOkIn=0 and Reset=1 signals a cold reset to the PHY.
//// 4. Wait a minimum of 8 cycles.
//// 5. Drive PwrOkIn to 1. Once the PwrOkIn is asserted (and Reset is still asserted),
////    DfiClk synchronously switches to any legal input frequency.
//// 6. Wait a minimum of 64 cycles. Note: This is the reset period for the PHY.
//// 7. Drive Reset to 0. Note: All DFI and APB inputs must be driven at valid reset states before the deassertion of Reset.
//// 8. Wait a minimum of 1 Cycle.
//// 9. Drive PRESETn_APB to 1 to de-assert reset on the ABP bus.
////10. The PHY is now in the reset state and is ready to accept APB transactions.
////
////##############################################################
//
//
dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy();

//// [dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy] End of dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy()
//

////##############################################################
////
//// Step (C) Initialize PHY Configuration
////
//// Load the required PHY configuration registers for the appropriate mode and memory configuration
////
////##############################################################
//

//// [phyinit_C_initPhyConfig] Start of dwc_ddrphy_phyinit_C_initPhyConfig()
//// [phyinit_C_initPhyConfig] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x1
dwc_ddrphy_apb_wr(0x200a6, 0x2); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for MASTER
dwc_ddrphy_apb_wr(0x20066, 0x0); // DWC_DDRPHYA_MASTER0_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for all DBYTEs
dwc_ddrphy_apb_wr(0x10066, 0x0); // DWC_DDRPHYA_DBYTE0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x11066, 0x0); // DWC_DDRPHYA_DBYTE1_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for all ANIBs
dwc_ddrphy_apb_wr(0x66, 0x0); // DWC_DDRPHYA_ANIB0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x1066, 0x0); // DWC_DDRPHYA_ANIB1_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x2066, 0x0); // DWC_DDRPHYA_ANIB2_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x3066, 0x0); // DWC_DDRPHYA_ANIB3_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x4066, 0x0); // DWC_DDRPHYA_ANIB4_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x5066, 0x0); // DWC_DDRPHYA_ANIB5_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x6066, 0x0); // DWC_DDRPHYA_ANIB6_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x7066, 0x0); // DWC_DDRPHYA_ANIB7_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x8066, 0x0); // DWC_DDRPHYA_ANIB8_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x9066, 0x0); // DWC_DDRPHYA_ANIB9_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl1::VshDAC to 0x31 for MASTER
dwc_ddrphy_apb_wr(0x20029, 0xc4); // DWC_DDRPHYA_MASTER0_base0_VREGCtrl1_p0
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl1::VshDAC to 0x31 for all DBYTEs
dwc_ddrphy_apb_wr(0x10029, 0xc4); // DWC_DDRPHYA_DBYTE0_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x11029, 0xc4); // DWC_DDRPHYA_DBYTE1_base0_VREGCtrl1_p0
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl1::VshDAC to 0x31 for all ANIBs
dwc_ddrphy_apb_wr(0x29, 0xc4); // DWC_DDRPHYA_ANIB0_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x1029, 0xc4); // DWC_DDRPHYA_ANIB1_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x2029, 0xc4); // DWC_DDRPHYA_ANIB2_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x3029, 0xc4); // DWC_DDRPHYA_ANIB3_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x4029, 0xc4); // DWC_DDRPHYA_ANIB4_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x5029, 0xc4); // DWC_DDRPHYA_ANIB5_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x6029, 0xc4); // DWC_DDRPHYA_ANIB6_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x7029, 0xc4); // DWC_DDRPHYA_ANIB7_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x8029, 0xc4); // DWC_DDRPHYA_ANIB8_base0_VREGCtrl1_p0
dwc_ddrphy_apb_wr(0x9029, 0xc4); // DWC_DDRPHYA_ANIB9_base0_VREGCtrl1_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxSlewRate::CsrTxSrc to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxSlewRate::TxPreDrvMode to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxSlewRate to 0x0
//// [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for TxSlewRate::CsrTxSrc are technology specific.
//// [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings

dwc_ddrphy_apb_wr(0x1005f, 0x0); // DWC_DDRPHYA_DBYTE0_base0_TxSlewRate_b0_p0
dwc_ddrphy_apb_wr(0x1015f, 0x0); // DWC_DDRPHYA_DBYTE0_base0_TxSlewRate_b1_p0
dwc_ddrphy_apb_wr(0x1105f, 0x0); // DWC_DDRPHYA_DBYTE1_base0_TxSlewRate_b0_p0
dwc_ddrphy_apb_wr(0x1115f, 0x0); // DWC_DDRPHYA_DBYTE1_base0_TxSlewRate_b1_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::ATxPreDrvMode to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 0 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 0 to 0x11e
dwc_ddrphy_apb_wr(0x55, 0x11e); // DWC_DDRPHYA_ANIB0_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 1 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 1 to 0x11e
dwc_ddrphy_apb_wr(0x1055, 0x11e); // DWC_DDRPHYA_ANIB1_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 2 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 2 to 0x11e
dwc_ddrphy_apb_wr(0x2055, 0x11e); // DWC_DDRPHYA_ANIB2_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 3 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 3 to 0x11e
dwc_ddrphy_apb_wr(0x3055, 0x11e); // DWC_DDRPHYA_ANIB3_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 4 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 4 to 0x11e
dwc_ddrphy_apb_wr(0x4055, 0x11e); // DWC_DDRPHYA_ANIB4_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 5 to 0x15a
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 5 to 0x15a
dwc_ddrphy_apb_wr(0x5055, 0x15a); // DWC_DDRPHYA_ANIB5_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 6 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 6 to 0x11e
dwc_ddrphy_apb_wr(0x6055, 0x11e); // DWC_DDRPHYA_ANIB6_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 7 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 7 to 0x11e
dwc_ddrphy_apb_wr(0x7055, 0x11e); // DWC_DDRPHYA_ANIB7_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 8 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 8 to 0x11e
dwc_ddrphy_apb_wr(0x8055, 0x11e); // DWC_DDRPHYA_ANIB8_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate::CsrATxSrc ANIB 9 to 0x11e
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxSlewRate ANIB 9 to 0x11e
dwc_ddrphy_apb_wr(0x9055, 0x11e); // DWC_DDRPHYA_ANIB9_base0_ATxSlewRate_p0
//// [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for ATxSlewRate::CsrATxSrc are technology specific.
//// [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings

//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming CalPreDriverOverride::CsrTxOvSrc to 0x172
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming CalPreDriverOverride::TxCalBaseN to 0x1
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming CalPreDriverOverride::TxCalBaseP to 0x1
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming CalPreDriverOverride to 0x372
//// [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for CalPreDriverOverride::CsrTxOvSrc are technology specific.
//// [phyinit_C_initPhyConfig] ### NOTE ### Please consult the "Output Slew Rate" section of HSpice Model App Note in specific technology for recommended settings

dwc_ddrphy_apb_wr(0x2008c, 0x372); // DWC_DDRPHYA_MASTER0_base0_CalPreDriverOverride
//// [phyinit_C_initPhyConfig] PUB revision is 0x0350.
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl2::PllFreqSel to 0x19 based on DfiClk frequency = 800.
dwc_ddrphy_apb_wr(0x200c5, 0x19); // DWC_DDRPHYA_MASTER0_base0_PllCtrl2_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl1::PllCpPropCtrl to 0x3 based on DfiClk frequency = 800.
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl1::PllCpIntCtrl to 0x1 based on DfiClk frequency = 800.
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl1 to 0x61 based on DfiClk frequency = 800.
dwc_ddrphy_apb_wr(0x200c7, 0x61); // DWC_DDRPHYA_MASTER0_base0_PllCtrl1_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllTestMode to 0x400f based on DfiClk frequency = 800.
dwc_ddrphy_apb_wr(0x200ca, 0x400f); // DWC_DDRPHYA_MASTER0_base0_PllTestMode_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl4::PllCpPropGsCtrl to 0x6 based on DfiClk frequency = 800.
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl4::PllCpIntGsCtrl to 0x12 based on DfiClk frequency = 800.
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming PllCtrl4 to 0xd2 based on DfiClk frequency = 800.
dwc_ddrphy_apb_wr(0x200cc, 0xd2); // DWC_DDRPHYA_MASTER0_base0_PllCtrl4_p0
//// [phyinit_C_initPhyConfig] ### NOTE ### Optimal setting for PllCtrl1 and PllTestMode are technology specific.
//// [phyinit_C_initPhyConfig] ### NOTE ### Please consult technology specific PHY Databook for recommended settings

//
////##############################################################
////
//// Program ARdPtrInitVal based on Frequency and PLL Bypass inputs
//// The values programmed here assume ideal properties of DfiClk
//// and Pclk including:
//// - DfiClk skew
//// - DfiClk jitter
//// - DfiClk PVT variations
//// - Pclk skew
//// - Pclk jitter
////
//// PLL Bypassed mode:
////     For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 2-5
////     For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-5
////
//// PLL Enabled mode:
////     For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-5
////     For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 0-5
////
////##############################################################
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ARdPtrInitVal to 0x1
dwc_ddrphy_apb_wr(0x2002e, 0x1); // DWC_DDRPHYA_MASTER0_base0_ARdPtrInitVal_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DisPtrInitClrTxTracking to 0x0
dwc_ddrphy_apb_wr(0x20051, 0x0); // DWC_DDRPHYA_MASTER0_base0_PtrInitTrackingModeCntrl_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::TwoTckRxDqsPre  to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::TwoTckTxDqsPre  to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::PositionDfeInit to 0x2
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::DDR5RxPreamble  to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::DDR5RxPostamble to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl                  to 0x8
dwc_ddrphy_apb_wr(0x20024, 0x8); // DWC_DDRPHYA_MASTER0_base0_DqsPreambleControl_p0
//// [phyinit_C_initPhyConfig] Programming DbyteDllModeCntrl::DllRxPreambleMode to 0x1
//// [phyinit_C_initPhyConfig] Programming DbyteDllModeCntrl::DllRxBurstLengthMode to 0x0
//// [phyinit_C_initPhyConfig] Programming DbyteDllModeCntrl to 0x2
dwc_ddrphy_apb_wr(0x2003a, 0x2); // DWC_DDRPHYA_MASTER0_base0_DbyteDllModeCntrl
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxOdtDrvStren::TxOdtStrenPu to 0x4
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxOdtDrvStren::TxOdtStrenPd to 0x0
dwc_ddrphy_apb_wr(0x1004d, 0x4); // DWC_DDRPHYA_DBYTE0_base0_TxOdtDrvStren_b0_p0
dwc_ddrphy_apb_wr(0x1014d, 0x4); // DWC_DDRPHYA_DBYTE0_base0_TxOdtDrvStren_b1_p0
dwc_ddrphy_apb_wr(0x1104d, 0x4); // DWC_DDRPHYA_DBYTE1_base0_TxOdtDrvStren_b0_p0
dwc_ddrphy_apb_wr(0x1114d, 0x4); // DWC_DDRPHYA_DBYTE1_base0_TxOdtDrvStren_b1_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxImpedance::ADrvStrenP to 0x3f
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxImpedance::ADrvStrenN to 0x3f
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxImpedance::ATxReserved13x12 to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxImpedance::ATxCalBaseN to 0x1
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming ATxImpedance::ATxCalBaseP to 0x1
dwc_ddrphy_apb_wr(0x43, 0xcfff); // DWC_DDRPHYA_ANIB0_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x1043, 0xcfff); // DWC_DDRPHYA_ANIB1_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x2043, 0xcfff); // DWC_DDRPHYA_ANIB2_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x3043, 0xcfff); // DWC_DDRPHYA_ANIB3_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x4043, 0xcfff); // DWC_DDRPHYA_ANIB4_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x5043, 0xcfff); // DWC_DDRPHYA_ANIB5_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x6043, 0xcfff); // DWC_DDRPHYA_ANIB6_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x7043, 0xcfff); // DWC_DDRPHYA_ANIB7_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x8043, 0xcfff); // DWC_DDRPHYA_ANIB8_base0_ATxImpedance
dwc_ddrphy_apb_wr(0x9043, 0xcfff); // DWC_DDRPHYA_ANIB9_base0_ATxImpedance
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl0::TxStrenEqHiPu to 0xc
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl0::TxStrenEqLoPd to 0xc
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl1::TxStrenPu to 0x3f
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl1::TxStrenPd to 0x3f
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl2::TxStrenEqLoPu to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl2::TxStrenEqHiPd to 0x0
dwc_ddrphy_apb_wr(0x10041, 0x30c); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl0_b0_p0
dwc_ddrphy_apb_wr(0x10049, 0xfff); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl1_b0_p0
dwc_ddrphy_apb_wr(0x1004b, 0x0); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl2_b0_p0
dwc_ddrphy_apb_wr(0x10141, 0x30c); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl0_b1_p0
dwc_ddrphy_apb_wr(0x10149, 0xfff); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl1_b1_p0
dwc_ddrphy_apb_wr(0x1014b, 0x0); // DWC_DDRPHYA_DBYTE0_base0_TxImpedanceCtrl2_b1_p0
dwc_ddrphy_apb_wr(0x11041, 0x30c); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl0_b0_p0
dwc_ddrphy_apb_wr(0x11049, 0xfff); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl1_b0_p0
dwc_ddrphy_apb_wr(0x1104b, 0x0); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl2_b0_p0
dwc_ddrphy_apb_wr(0x11141, 0x30c); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl0_b1_p0
dwc_ddrphy_apb_wr(0x11149, 0xfff); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl1_b1_p0
dwc_ddrphy_apb_wr(0x1114b, 0x0); // DWC_DDRPHYA_DBYTE1_base0_TxImpedanceCtrl2_b1_p0
//// [phyinit_C_initPhyConfig] Programming DfiMode to 0x1
dwc_ddrphy_apb_wr(0x20018, 0x1); // DWC_DDRPHYA_MASTER0_base0_DfiMode
//// [phyinit_C_initPhyConfig] Programming DfiCAMode to 0x2
dwc_ddrphy_apb_wr(0x20075, 0x2); // DWC_DDRPHYA_MASTER0_base0_DfiCAMode
//// [phyinit_C_initPhyConfig] Programming CalDrvStr0::CalDrvStrPd50 to 0x2
//// [phyinit_C_initPhyConfig] Programming CalDrvStr0::CalDrvStrPu50 to 0x2
dwc_ddrphy_apb_wr(0x20050, 0x82); // DWC_DDRPHYA_MASTER0_base0_CalDrvStr0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming CalUclkInfo::CalUClkTicksPer1uS to 0x320
dwc_ddrphy_apb_wr(0x20008, 0x320); // DWC_DDRPHYA_MASTER0_base0_CalUclkInfo_p0
//// [phyinit_C_initPhyConfig] Programming CalRate::CalInterval to 0x9
//// [phyinit_C_initPhyConfig] Programming CalRate::CalOnce to 0x0
dwc_ddrphy_apb_wr(0x20088, 0x9); // DWC_DDRPHYA_MASTER0_base0_CalRate
//// [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal::GlobalVrefInSel to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal::GlobalVrefInDAC to 0x1f
//// [phyinit_C_initPhyConfig] Pstate=0, Programming VrefInGlobal to 0xf8
dwc_ddrphy_apb_wr(0x200b2, 0xf8); // DWC_DDRPHYA_MASTER0_base0_VrefInGlobal_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl (Byte=0, Upper/Lower=0) to 0x2500
dwc_ddrphy_apb_wr(0x10043, 0x2500); // DWC_DDRPHYA_DBYTE0_base0_DqDqsRcvCntrl_b0_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl (Byte=0, Upper/Lower=1) to 0x2500
dwc_ddrphy_apb_wr(0x10143, 0x2500); // DWC_DDRPHYA_DBYTE0_base0_DqDqsRcvCntrl_b1_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl (Byte=1, Upper/Lower=0) to 0x2500
dwc_ddrphy_apb_wr(0x11043, 0x2500); // DWC_DDRPHYA_DBYTE1_base0_DqDqsRcvCntrl_b0_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl (Byte=1, Upper/Lower=1) to 0x2500
dwc_ddrphy_apb_wr(0x11143, 0x2500); // DWC_DDRPHYA_DBYTE1_base0_DqDqsRcvCntrl_b1_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Programming DqDqsRcvCntrl2 to 0x1c
dwc_ddrphy_apb_wr(0x1004c, 0x1c); // DWC_DDRPHYA_DBYTE0_base0_DqDqsRcvCntrl2_p0
dwc_ddrphy_apb_wr(0x1104c, 0x1c); // DWC_DDRPHYA_DBYTE1_base0_DqDqsRcvCntrl2_p0
//// [phyinit_C_initPhyConfig] Programming ATxOdtDrvStren of ANIB_0 to 0x0
dwc_ddrphy_apb_wr(0x42, 0x0); // DWC_DDRPHYA_ANIB0_base0_ATxOdtDrvStren
//// [phyinit_C_initPhyConfig] Programming ATxOdtDrvStren of ANIB_0 to 0x0
dwc_ddrphy_apb_wr(0x42, 0x0); // DWC_DDRPHYA_ANIB0_base0_ATxOdtDrvStren
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TristateModeCA::DisDynAdrTri_p0 to 0x0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming TristateModeCA::DDR2TMode_p0 to 0x0
dwc_ddrphy_apb_wr(0x20019, 0x4); // DWC_DDRPHYA_MASTER0_base0_TristateModeCA_p0
//// [phyinit_C_initPhyConfig] Programming DfiFreqXlat*
dwc_ddrphy_apb_wr(0x200f0, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat0
dwc_ddrphy_apb_wr(0x200f1, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat1
dwc_ddrphy_apb_wr(0x200f2, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat2
dwc_ddrphy_apb_wr(0x200f3, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat3
dwc_ddrphy_apb_wr(0x200f4, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat4
dwc_ddrphy_apb_wr(0x200f5, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat5
dwc_ddrphy_apb_wr(0x200f6, 0x5555); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat6
dwc_ddrphy_apb_wr(0x200f7, 0xf000); // DWC_DDRPHYA_MASTER0_base0_DfiFreqXlat7
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY0 to 0x64
dwc_ddrphy_apb_wr(0x2000b, 0x64); // DWC_DDRPHYA_MASTER0_base0_Seq0BDLY0_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY1 to 0xc8
dwc_ddrphy_apb_wr(0x2000c, 0xc8); // DWC_DDRPHYA_MASTER0_base0_Seq0BDLY1_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY2 to 0x2bc
dwc_ddrphy_apb_wr(0x2000d, 0x2bc); // DWC_DDRPHYA_MASTER0_base0_Seq0BDLY2_p0
//// [phyinit_C_initPhyConfig] Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY3 to 0x2c
dwc_ddrphy_apb_wr(0x2000e, 0x2c); // DWC_DDRPHYA_MASTER0_base0_Seq0BDLY3_p0
//// [phyinit_C_initPhyConfig] Disabling DBYTE 0 Lane 8 (DBI) Receiver to save power.
dwc_ddrphy_apb_wr(0x1004a, 0x500); // DWC_DDRPHYA_DBYTE0_base0_DqDqsRcvCntrl1
//// [phyinit_C_initPhyConfig] Disabling DBYTE 1 Lane 8 (DBI) Receiver to save power.
dwc_ddrphy_apb_wr(0x1104a, 0x500); // DWC_DDRPHYA_DBYTE1_base0_DqDqsRcvCntrl1
//// [phyinit_C_initPhyConfig] Programming MasterX4Config::X4TG to 0x0
dwc_ddrphy_apb_wr(0x20025, 0x0); // DWC_DDRPHYA_MASTER0_base0_MasterX4Config
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming GPR7(csrAlertRecovery) to 0x0
dwc_ddrphy_apb_wr(0x90307, 0x0); // DWC_DDRPHYA_INITENG0_base0_Seq0BGPR7_p0
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DMIPinPresent::RdDbiEnabled to 0x0
dwc_ddrphy_apb_wr(0x2002d, 0x0); // DWC_DDRPHYA_MASTER0_base0_DMIPinPresent_p0
// [phyinit_C_initPhyConfig] Programming TimingModeCntrl::Dly64Prec to 0x0
dwc_ddrphy_apb_wr(0x20040, 0x0); // DWC_DDRPHYA_MASTER0_base0_TimingModeCntrl
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x1 for MASTER
dwc_ddrphy_apb_wr(0x20066, 0x1); // DWC_DDRPHYA_MASTER0_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x1 for all DBYTEs
dwc_ddrphy_apb_wr(0x10066, 0x1); // DWC_DDRPHYA_DBYTE0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x11066, 0x1); // DWC_DDRPHYA_DBYTE1_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x1 for all ANIBs
dwc_ddrphy_apb_wr(0x66, 0x1); // DWC_DDRPHYA_ANIB0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x1066, 0x1); // DWC_DDRPHYA_ANIB1_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x2066, 0x1); // DWC_DDRPHYA_ANIB2_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x3066, 0x1); // DWC_DDRPHYA_ANIB3_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x4066, 0x1); // DWC_DDRPHYA_ANIB4_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x5066, 0x1); // DWC_DDRPHYA_ANIB5_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x6066, 0x1); // DWC_DDRPHYA_ANIB6_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x7066, 0x1); // DWC_DDRPHYA_ANIB7_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x8066, 0x1); // DWC_DDRPHYA_ANIB8_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x9066, 0x1); // DWC_DDRPHYA_ANIB9_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming AcClkDLLControl to 0x1080
dwc_ddrphy_apb_wr(0x200ea, 0x1080); // DWC_DDRPHYA_MASTER0_base0_AcClkDLLControl_p0
// [phyinit_C_initPhyConfig] Programming ArcPmuEccCtl to 0x0
dwc_ddrphy_apb_wr(0xc0086, 0x0); // DWC_DDRPHYA_DRTUB0_ArcPmuEccCtl
// [phyinit_C_initPhyConfig] Programming VREGCtrl2 to 0x9820 for MASTER
dwc_ddrphy_apb_wr(0x2002b, 0x9820); // DWC_DDRPHYA_MASTER0_base0_VREGCtrl2
// [phyinit_C_initPhyConfig] Programming VREGCtrl2 to 0x8020 for all DBYTEs
dwc_ddrphy_apb_wr(0x1002b, 0x8020); // DWC_DDRPHYA_DBYTE0_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x1102b, 0x8020); // DWC_DDRPHYA_DBYTE1_base0_VREGCtrl2
// [phyinit_C_initPhyConfig] Programming VREGCtrl2 to 0x8020 for all ANIBs
dwc_ddrphy_apb_wr(0x2b, 0x8020); // DWC_DDRPHYA_ANIB0_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x102b, 0x8020); // DWC_DDRPHYA_ANIB1_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x202b, 0x8020); // DWC_DDRPHYA_ANIB2_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x302b, 0x8020); // DWC_DDRPHYA_ANIB3_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x402b, 0x8020); // DWC_DDRPHYA_ANIB4_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x502b, 0x8020); // DWC_DDRPHYA_ANIB5_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x602b, 0x8020); // DWC_DDRPHYA_ANIB6_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x702b, 0x8020); // DWC_DDRPHYA_ANIB7_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x802b, 0x8020); // DWC_DDRPHYA_ANIB8_base0_VREGCtrl2
dwc_ddrphy_apb_wr(0x902b, 0x8020); // DWC_DDRPHYA_ANIB9_base0_VREGCtrl2
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for MASTER
dwc_ddrphy_apb_wr(0x20066, 0x0); // DWC_DDRPHYA_MASTER0_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for all DBYTEs
dwc_ddrphy_apb_wr(0x10066, 0x0); // DWC_DDRPHYA_DBYTE0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x11066, 0x0); // DWC_DDRPHYA_DBYTE1_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming VREGCtrl3::VshCtrlUpdate to 0x0 for all ANIBs
dwc_ddrphy_apb_wr(0x66, 0x0); // DWC_DDRPHYA_ANIB0_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x1066, 0x0); // DWC_DDRPHYA_ANIB1_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x2066, 0x0); // DWC_DDRPHYA_ANIB2_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x3066, 0x0); // DWC_DDRPHYA_ANIB3_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x4066, 0x0); // DWC_DDRPHYA_ANIB4_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x5066, 0x0); // DWC_DDRPHYA_ANIB5_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x6066, 0x0); // DWC_DDRPHYA_ANIB6_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x7066, 0x0); // DWC_DDRPHYA_ANIB7_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x8066, 0x0); // DWC_DDRPHYA_ANIB8_base0_VREGCtrl3
dwc_ddrphy_apb_wr(0x9066, 0x0); // DWC_DDRPHYA_ANIB9_base0_VREGCtrl3
// [phyinit_C_initPhyConfig] Programming VrefDAC0 to 0x3f for all DBYTEs and lanes
// [phyinit_C_initPhyConfig] Programming VrefDAC1 to 0x3f for all DBYTEs and lanes
// [phyinit_C_initPhyConfig] Programming VrefDAC2 to 0x3f for all DBYTEs and lanes
// [phyinit_C_initPhyConfig] Programming VrefDAC3 to 0x3f for all DBYTEs and lanes
dwc_ddrphy_apb_wr(0x10040, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r0_p0
dwc_ddrphy_apb_wr(0x10030, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r0
dwc_ddrphy_apb_wr(0x10050, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r0
dwc_ddrphy_apb_wr(0x10060, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r0
dwc_ddrphy_apb_wr(0x10140, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r1_p0
dwc_ddrphy_apb_wr(0x10130, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r1
dwc_ddrphy_apb_wr(0x10150, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r1
dwc_ddrphy_apb_wr(0x10160, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r1
dwc_ddrphy_apb_wr(0x10240, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r2_p0
dwc_ddrphy_apb_wr(0x10230, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r2
dwc_ddrphy_apb_wr(0x10250, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r2
dwc_ddrphy_apb_wr(0x10260, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r2
dwc_ddrphy_apb_wr(0x10340, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r3_p0
dwc_ddrphy_apb_wr(0x10330, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r3
dwc_ddrphy_apb_wr(0x10350, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r3
dwc_ddrphy_apb_wr(0x10360, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r3
dwc_ddrphy_apb_wr(0x10440, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r4_p0
dwc_ddrphy_apb_wr(0x10430, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r4
dwc_ddrphy_apb_wr(0x10450, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r4
dwc_ddrphy_apb_wr(0x10460, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r4
dwc_ddrphy_apb_wr(0x10540, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r5_p0
dwc_ddrphy_apb_wr(0x10530, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r5
dwc_ddrphy_apb_wr(0x10550, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r5
dwc_ddrphy_apb_wr(0x10560, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r5
dwc_ddrphy_apb_wr(0x10640, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r6_p0
dwc_ddrphy_apb_wr(0x10630, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r6
dwc_ddrphy_apb_wr(0x10650, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r6
dwc_ddrphy_apb_wr(0x10660, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r6
dwc_ddrphy_apb_wr(0x10740, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r7_p0
dwc_ddrphy_apb_wr(0x10730, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r7
dwc_ddrphy_apb_wr(0x10750, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r7
dwc_ddrphy_apb_wr(0x10760, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r7
dwc_ddrphy_apb_wr(0x10840, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC0_r8_p0
dwc_ddrphy_apb_wr(0x10830, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC1_r8
dwc_ddrphy_apb_wr(0x10850, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC2_r8
dwc_ddrphy_apb_wr(0x10860, 0x3f); // DWC_DDRPHYA_DBYTE0_base0_VrefDAC3_r8
dwc_ddrphy_apb_wr(0x11040, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r0_p0
dwc_ddrphy_apb_wr(0x11030, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r0
dwc_ddrphy_apb_wr(0x11050, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r0
dwc_ddrphy_apb_wr(0x11060, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r0
dwc_ddrphy_apb_wr(0x11140, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r1_p0
dwc_ddrphy_apb_wr(0x11130, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r1
dwc_ddrphy_apb_wr(0x11150, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r1
dwc_ddrphy_apb_wr(0x11160, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r1
dwc_ddrphy_apb_wr(0x11240, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r2_p0
dwc_ddrphy_apb_wr(0x11230, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r2
dwc_ddrphy_apb_wr(0x11250, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r2
dwc_ddrphy_apb_wr(0x11260, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r2
dwc_ddrphy_apb_wr(0x11340, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r3_p0
dwc_ddrphy_apb_wr(0x11330, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r3
dwc_ddrphy_apb_wr(0x11350, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r3
dwc_ddrphy_apb_wr(0x11360, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r3
dwc_ddrphy_apb_wr(0x11440, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r4_p0
dwc_ddrphy_apb_wr(0x11430, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r4
dwc_ddrphy_apb_wr(0x11450, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r4
dwc_ddrphy_apb_wr(0x11460, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r4
dwc_ddrphy_apb_wr(0x11540, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r5_p0
dwc_ddrphy_apb_wr(0x11530, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r5
dwc_ddrphy_apb_wr(0x11550, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r5
dwc_ddrphy_apb_wr(0x11560, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r5
dwc_ddrphy_apb_wr(0x11640, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r6_p0
dwc_ddrphy_apb_wr(0x11630, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r6
dwc_ddrphy_apb_wr(0x11650, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r6
dwc_ddrphy_apb_wr(0x11660, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r6
dwc_ddrphy_apb_wr(0x11740, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r7_p0
dwc_ddrphy_apb_wr(0x11730, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r7
dwc_ddrphy_apb_wr(0x11750, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r7
dwc_ddrphy_apb_wr(0x11760, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r7
dwc_ddrphy_apb_wr(0x11840, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC0_r8_p0
dwc_ddrphy_apb_wr(0x11830, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC1_r8
dwc_ddrphy_apb_wr(0x11850, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC2_r8
dwc_ddrphy_apb_wr(0x11860, 0x3f); // DWC_DDRPHYA_DBYTE1_base0_VrefDAC3_r8
//// [phyinit_C_initPhyConfig] Pstate=0, Memclk=1600MHz, Programming DfiFreqRatio_p0 to 0x1
dwc_ddrphy_apb_wr(0x200fa, 0x1); // DWC_DDRPHYA_MASTER0_base0_DfiFreqRatio_p0
//// [phyinit_C_initPhyConfig] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x0
dwc_ddrphy_apb_wr(0x200a6, 0x0); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
//// [phyinit_C_initPhyConfig] Programming AForceTriCont (anib=0) to 0xc
dwc_ddrphy_apb_wr(0x28, 0xc); // DWC_DDRPHYA_ANIB0_base0_AForceTriCont
//// [phyinit_C_initPhyConfig] End of dwc_ddrphy_phyinit_C_initPhyConfig()
//
//
////##############################################################
////
//// dwc_ddrphy_phyinit_userCustom_customPreTrain is a user-editable function.
////
//// The purpose of dwc_ddrphy_phyinit_userCustom_customPreTrain() is to override any
//// any message block fields calculated by Phyinit in dwc_ddrphy_phyinit_calcMb() or to
//// override any CSR values programmed by Phyinit in dwc_ddrphy_phyinit_C_initPhyConfig().
//// This function is executed before training and thus any override here might affect
//// training result.
////
//// IMPORTANT: in this function, user shall not override any values in userInputBasic and
//// userInputAdvanced data structures. Use dwc_ddrphy_phyinit_userCustom_overrideUserInput()
//// to modify values in those data structures.
////
////##############################################################
//
//// [phyinit_userCustom_customPreTrain] Start of dwc_ddrphy_phyinit_userCustom_customPreTrain()
//// [phyinit_userCustom_customPreTrain] End of dwc_ddrphy_phyinit_userCustom_customPreTrain()
//

////##############################################################
////
//// Training firmware is *NOT* executed. This function replaces these steps
//// in the PHY Initialization sequence:
////
////  4.3.1.4(E) Set the PHY input clocks to the desired frequency
////  4.3.1.5(F) Write the Message Block parameters for the training firmware
////  4.3.1.6(G) Execute the Training Firmware
////  4.3.1.7(H) Read the Message Block results
////
////##############################################################
//

//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Start of dwc_ddrphy_phyinit_progCsrSkipTrain()
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x1
dwc_ddrphy_apb_wr(0x200a6, 0x2); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] StepsPerUI = 32
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] NumRank_total = 1
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] TG_active[0] = 1
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] TG_active[1] = 0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] TG_active[2] = 0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] TG_active[3] = 0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_CK [pstate=0][tg=0] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_DQ [pstate=0][tg=0] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_CK [pstate=0][tg=1] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_DQ [pstate=0][tg=1] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_CK [pstate=0][tg=2] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_DQ [pstate=0][tg=2] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_CK [pstate=0][tg=3] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_DQ [pstate=0][tg=3] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_CK_max [pstate=0] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tDIMM_DQ_max [pstate=0] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] tCASL_add_max[pstate=0] = 0 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] PHY_Rx_Fifo_Dly = 1675 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] PHY_Tx_Insertion_Dly = 111 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] PHY_Rx_Insertion_Dly = 111 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming DFIMRL to 0x5 Margin is 2
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming HwtMRL to 0x5
dwc_ddrphy_apb_wr(0x10020, 0x5); // DWC_DDRPHYA_DBYTE0_base0_DFIMRL_p0
dwc_ddrphy_apb_wr(0x11020, 0x5); // DWC_DDRPHYA_DBYTE1_base0_DFIMRL_p0
dwc_ddrphy_apb_wr(0x20020, 0x5); // DWC_DDRPHYA_MASTER0_base0_HwtMRL_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, TxDqsDly_in_ps = 1250.000000 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming TxDqsDlyTg0 to 0x100
dwc_ddrphy_apb_wr(0x100d0, 0x100); // DWC_DDRPHYA_DBYTE0_base0_TxDqsDlyTg0_u0_p0
dwc_ddrphy_apb_wr(0x101d0, 0x100); // DWC_DDRPHYA_DBYTE0_base0_TxDqsDlyTg0_u1_p0
dwc_ddrphy_apb_wr(0x110d0, 0x100); // DWC_DDRPHYA_DBYTE1_base0_TxDqsDlyTg0_u0_p0
dwc_ddrphy_apb_wr(0x111d0, 0x100); // DWC_DDRPHYA_DBYTE1_base0_TxDqsDlyTg0_u1_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, TxDqDly_in_ps = 156.250000 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming TxDqDlyTg0 to 0x10
dwc_ddrphy_apb_wr(0x100c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r0_p0
dwc_ddrphy_apb_wr(0x101c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r1_p0
dwc_ddrphy_apb_wr(0x102c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r2_p0
dwc_ddrphy_apb_wr(0x103c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r3_p0
dwc_ddrphy_apb_wr(0x104c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r4_p0
dwc_ddrphy_apb_wr(0x105c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r5_p0
dwc_ddrphy_apb_wr(0x106c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r6_p0
dwc_ddrphy_apb_wr(0x107c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r7_p0
dwc_ddrphy_apb_wr(0x108c0, 0x10); // DWC_DDRPHYA_DBYTE0_base0_TxDqDlyTg0_r8_p0
dwc_ddrphy_apb_wr(0x110c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r0_p0
dwc_ddrphy_apb_wr(0x111c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r1_p0
dwc_ddrphy_apb_wr(0x112c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r2_p0
dwc_ddrphy_apb_wr(0x113c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r3_p0
dwc_ddrphy_apb_wr(0x114c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r4_p0
dwc_ddrphy_apb_wr(0x115c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r5_p0
dwc_ddrphy_apb_wr(0x116c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r6_p0
dwc_ddrphy_apb_wr(0x117c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r7_p0
dwc_ddrphy_apb_wr(0x118c0, 0x10); // DWC_DDRPHYA_DBYTE1_base0_TxDqDlyTg0_r8_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, _progCsrSkipTrain D4RxPreambleLength[pstate] 0x0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, RxEnDly_in_ps = 1250.000000 ps
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming RxEnDlyTg0 to 0x100
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, _progCsrSkipTrain RxEnDly_preambleBackoff_ps    0.00
dwc_ddrphy_apb_wr(0x10080, 0x100); // DWC_DDRPHYA_DBYTE0_base0_RxEnDlyTg0_u0_p0
dwc_ddrphy_apb_wr(0x10180, 0x100); // DWC_DDRPHYA_DBYTE0_base0_RxEnDlyTg0_u1_p0
dwc_ddrphy_apb_wr(0x11080, 0x100); // DWC_DDRPHYA_DBYTE1_base0_RxEnDlyTg0_u0_p0
dwc_ddrphy_apb_wr(0x11180, 0x100); // DWC_DDRPHYA_DBYTE1_base0_RxEnDlyTg0_u1_p0
// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming RxClkDlyTg0 to 0x14
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming HwtCAMode to 0x2
dwc_ddrphy_apb_wr(0x20077, 0x2); // DWC_DDRPHYA_MASTER0_base0_HwtCAMode
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming DllGainCtl::DllGainIV=0x3, DllGainTV=0x5
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming DllGainCtl to 0x53
dwc_ddrphy_apb_wr(0x2007c, 0x53); // DWC_DDRPHYA_MASTER0_base0_DllGainCtl_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming DllLockParam::LcdlSeed0 to 129
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Pstate=0, Memclk=1600MHz, Programming DllLockParam to 0x81
dwc_ddrphy_apb_wr(0x2007d, 0x81); // DWC_DDRPHYA_MASTER0_base0_DllLockParam_p0
dwc_ddrphy_apb_wr(0x20071, 0x2); // DWC_DDRPHYA_MASTER0_base0_DllTrainParam_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming AcsmCtrl23 to 0x10f
dwc_ddrphy_apb_wr(0x400c0, 0x10f); // DWC_DDRPHYA_ACSM0_base0_AcsmCtrl23
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming GPR3/PllCtrl3::PllDacValIn to 0x10
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming GPR3/PllCtrl3::PllMaxRange to 0x1f
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming GPR3 for PllCtrl3::PllForceCal to 0x1
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming GPR3 for PllCtrl3::PllEnCal to 0x1
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming PllCtrl3 to 0x21f0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming GPR3 to 0xe1f0
dwc_ddrphy_apb_wr(0x200cb, 0x21f0); // DWC_DDRPHYA_MASTER0_base0_PllCtrl3
dwc_ddrphy_apb_wr(0x90303, 0xe1f0); // DWC_DDRPHYA_INITENG0_base0_Seq0BGPR3_p0
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x0
dwc_ddrphy_apb_wr(0x200a6, 0x0); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
//// [dwc_ddrphy_phyinit_progCsrSkipTrain] End of dwc_ddrphy_phyinit_progCsrSkipTrain()
//// [initRuntimeConfigEnableBits] Start of initRuntimeConfigEnableBits()
//// [initRuntimeConfigEnableBits] enableBits[0] = 0x00000009
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_A0 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_A1 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_A2 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_A3 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] enableBits[1] = 0x00000000
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_B0 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_B1 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_B2 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] WR_RD_RTT_PARK_B3 = 0x000000ff, rtt_required = 0x0000000f
//// [initRuntimeConfigEnableBits] enableBits[2] = 0x00000000
//// [initRuntimeConfigEnableBits] End of initRuntimeConfigEnableBits()
//// [phyinit_I_loadPIEImage] Start of dwc_ddrphy_phyinit_I_loadPIEImage()
//
//
////##############################################################
////
//// 4.3.9(I) Load PHY Init Engine Image
////
//// Load the PHY Initialization Engine memory with the provided initialization sequence.
////
////##############################################################
//
//
//// Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.
//// This allows the memory controller unrestricted access to the configuration CSRs.
dwc_ddrphy_apb_wr(0xd0000, 0x0); // DWC_DDRPHYA_APBONLY0_MicroContMuxSel
//// [phyinit_userCustom_wait] Wait 40 DfiClks
//// [phyinit_I_loadPIEImage] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x1
dwc_ddrphy_apb_wr(0x200a6, 0x2); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
//// [phyinit_I_loadPIEImage] Programming PIE Production Code
//// [phyinit_LoadPIECodeSections] Start of dwc_ddrphy_phyinit_LoadPIECodeSections()
//// [phyinit_LoadPIECodeSections] Moving start address from 0 to 90000
dwc_ddrphy_apb_wr(0x90000, 0x10); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b0s0
dwc_ddrphy_apb_wr(0x90001, 0x400); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b0s1
dwc_ddrphy_apb_wr(0x90002, 0x10e); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b0s2
dwc_ddrphy_apb_wr(0x90003, 0x0); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b1s0
dwc_ddrphy_apb_wr(0x90004, 0x0); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b1s1
dwc_ddrphy_apb_wr(0x90005, 0x8); // DWC_DDRPHYA_INITENG0_base0_PreSequenceReg0b1s2
//// [phyinit_LoadPIECodeSections] Moving start address from 90006 to 90029
dwc_ddrphy_apb_wr(0x90029, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b0s0
dwc_ddrphy_apb_wr(0x9002a, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b0s1
dwc_ddrphy_apb_wr(0x9002b, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b0s2
dwc_ddrphy_apb_wr(0x9002c, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b1s0
dwc_ddrphy_apb_wr(0x9002d, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b1s1
dwc_ddrphy_apb_wr(0x9002e, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b1s2
dwc_ddrphy_apb_wr(0x9002f, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b2s0
dwc_ddrphy_apb_wr(0x90030, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b2s1
dwc_ddrphy_apb_wr(0x90031, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b2s2
dwc_ddrphy_apb_wr(0x90032, 0xb); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b3s0
dwc_ddrphy_apb_wr(0x90033, 0x480); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b3s1
dwc_ddrphy_apb_wr(0x90034, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b3s2
dwc_ddrphy_apb_wr(0x90035, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b4s0
dwc_ddrphy_apb_wr(0x90036, 0x448); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b4s1
dwc_ddrphy_apb_wr(0x90037, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b4s2
dwc_ddrphy_apb_wr(0x90038, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b5s0
dwc_ddrphy_apb_wr(0x90039, 0x478); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b5s1
dwc_ddrphy_apb_wr(0x9003a, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b5s2
dwc_ddrphy_apb_wr(0x9003b, 0x2); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b6s0
dwc_ddrphy_apb_wr(0x9003c, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b6s1
dwc_ddrphy_apb_wr(0x9003d, 0x139); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b6s2
dwc_ddrphy_apb_wr(0x9003e, 0xf); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b7s0
dwc_ddrphy_apb_wr(0x9003f, 0x7c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b7s1
dwc_ddrphy_apb_wr(0x90040, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b7s2
dwc_ddrphy_apb_wr(0x90041, 0x107); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b8s0
dwc_ddrphy_apb_wr(0x90042, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b8s1
dwc_ddrphy_apb_wr(0x90043, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b8s2
dwc_ddrphy_apb_wr(0x90044, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b9s0
dwc_ddrphy_apb_wr(0x90045, 0xe0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b9s1
dwc_ddrphy_apb_wr(0x90046, 0x139); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b9s2
dwc_ddrphy_apb_wr(0x90047, 0x147); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b10s0
dwc_ddrphy_apb_wr(0x90048, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b10s1
dwc_ddrphy_apb_wr(0x90049, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b10s2
dwc_ddrphy_apb_wr(0x9004a, 0x14f); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b11s0
dwc_ddrphy_apb_wr(0x9004b, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b11s1
dwc_ddrphy_apb_wr(0x9004c, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b11s2
dwc_ddrphy_apb_wr(0x9004d, 0x7); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b12s0
dwc_ddrphy_apb_wr(0x9004e, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b12s1
dwc_ddrphy_apb_wr(0x9004f, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b12s2
dwc_ddrphy_apb_wr(0x90050, 0x47); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b13s0
dwc_ddrphy_apb_wr(0x90051, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b13s1
dwc_ddrphy_apb_wr(0x90052, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b13s2
dwc_ddrphy_apb_wr(0x90053, 0x4f); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b14s0
dwc_ddrphy_apb_wr(0x90054, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b14s1
dwc_ddrphy_apb_wr(0x90055, 0x179); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b14s2
dwc_ddrphy_apb_wr(0x90056, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b15s0
dwc_ddrphy_apb_wr(0x90057, 0x7c8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b15s1
dwc_ddrphy_apb_wr(0x90058, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b15s2
dwc_ddrphy_apb_wr(0x90059, 0x11); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b16s0
dwc_ddrphy_apb_wr(0x9005a, 0x530); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b16s1
dwc_ddrphy_apb_wr(0x9005b, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b16s2
dwc_ddrphy_apb_wr(0x9005c, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b17s0
dwc_ddrphy_apb_wr(0x9005d, 0x1); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b17s1
dwc_ddrphy_apb_wr(0x9005e, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b17s2
dwc_ddrphy_apb_wr(0x9005f, 0x14f); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b18s0
dwc_ddrphy_apb_wr(0x90060, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b18s1
dwc_ddrphy_apb_wr(0x90061, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b18s2
dwc_ddrphy_apb_wr(0x90062, 0x2); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b19s0
dwc_ddrphy_apb_wr(0x90063, 0x45a); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b19s1
dwc_ddrphy_apb_wr(0x90064, 0x9); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b19s2
dwc_ddrphy_apb_wr(0x90065, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b20s0
dwc_ddrphy_apb_wr(0x90066, 0x530); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b20s1
dwc_ddrphy_apb_wr(0x90067, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b20s2
dwc_ddrphy_apb_wr(0x90068, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b21s0
dwc_ddrphy_apb_wr(0x90069, 0x65a); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b21s1
dwc_ddrphy_apb_wr(0x9006a, 0x9); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b21s2
dwc_ddrphy_apb_wr(0x9006b, 0x41); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b22s0
dwc_ddrphy_apb_wr(0x9006c, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b22s1
dwc_ddrphy_apb_wr(0x9006d, 0x179); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b22s2
dwc_ddrphy_apb_wr(0x9006e, 0x1); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b23s0
dwc_ddrphy_apb_wr(0x9006f, 0x618); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b23s1
dwc_ddrphy_apb_wr(0x90070, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b23s2
dwc_ddrphy_apb_wr(0x90071, 0x40c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b24s0
dwc_ddrphy_apb_wr(0x90072, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b24s1
dwc_ddrphy_apb_wr(0x90073, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b24s2
dwc_ddrphy_apb_wr(0x90074, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b25s0
dwc_ddrphy_apb_wr(0x90075, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b25s1
dwc_ddrphy_apb_wr(0x90076, 0x48); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b25s2
dwc_ddrphy_apb_wr(0x90077, 0x4040); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b26s0
dwc_ddrphy_apb_wr(0x90078, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b26s1
dwc_ddrphy_apb_wr(0x90079, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b26s2
dwc_ddrphy_apb_wr(0x9007a, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b27s0
dwc_ddrphy_apb_wr(0x9007b, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b27s1
dwc_ddrphy_apb_wr(0x9007c, 0x48); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b27s2
dwc_ddrphy_apb_wr(0x9007d, 0x40); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b28s0
dwc_ddrphy_apb_wr(0x9007e, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b28s1
dwc_ddrphy_apb_wr(0x9007f, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b28s2
dwc_ddrphy_apb_wr(0x90080, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b29s0
dwc_ddrphy_apb_wr(0x90081, 0x658); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b29s1
dwc_ddrphy_apb_wr(0x90082, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b29s2
dwc_ddrphy_apb_wr(0x90083, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b30s0
dwc_ddrphy_apb_wr(0x90084, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b30s1
dwc_ddrphy_apb_wr(0x90085, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b30s2
dwc_ddrphy_apb_wr(0x90086, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b31s0
dwc_ddrphy_apb_wr(0x90087, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b31s1
dwc_ddrphy_apb_wr(0x90088, 0x78); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b31s2
dwc_ddrphy_apb_wr(0x90089, 0x549); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b32s0
dwc_ddrphy_apb_wr(0x9008a, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b32s1
dwc_ddrphy_apb_wr(0x9008b, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b32s2
dwc_ddrphy_apb_wr(0x9008c, 0xd49); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b33s0
dwc_ddrphy_apb_wr(0x9008d, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b33s1
dwc_ddrphy_apb_wr(0x9008e, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b33s2
dwc_ddrphy_apb_wr(0x9008f, 0x94c); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b34s0
dwc_ddrphy_apb_wr(0x90090, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b34s1
dwc_ddrphy_apb_wr(0x90091, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b34s2
dwc_ddrphy_apb_wr(0x90092, 0x94c); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b35s0
dwc_ddrphy_apb_wr(0x90093, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b35s1
dwc_ddrphy_apb_wr(0x90094, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b35s2
dwc_ddrphy_apb_wr(0x90095, 0x442); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b36s0
dwc_ddrphy_apb_wr(0x90096, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b36s1
dwc_ddrphy_apb_wr(0x90097, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b36s2
dwc_ddrphy_apb_wr(0x90098, 0x42); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b37s0
dwc_ddrphy_apb_wr(0x90099, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b37s1
dwc_ddrphy_apb_wr(0x9009a, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b37s2
dwc_ddrphy_apb_wr(0x9009b, 0x1); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b38s0
dwc_ddrphy_apb_wr(0x9009c, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b38s1
dwc_ddrphy_apb_wr(0x9009d, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b38s2
dwc_ddrphy_apb_wr(0x9009e, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b39s0
dwc_ddrphy_apb_wr(0x9009f, 0xe0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b39s1
dwc_ddrphy_apb_wr(0x900a0, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b39s2
dwc_ddrphy_apb_wr(0x900a1, 0xa); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b40s0
dwc_ddrphy_apb_wr(0x900a2, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b40s1
dwc_ddrphy_apb_wr(0x900a3, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b40s2
dwc_ddrphy_apb_wr(0x900a4, 0x9); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b41s0
dwc_ddrphy_apb_wr(0x900a5, 0x3c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b41s1
dwc_ddrphy_apb_wr(0x900a6, 0x149); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b41s2
dwc_ddrphy_apb_wr(0x900a7, 0x9); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b42s0
dwc_ddrphy_apb_wr(0x900a8, 0x3c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b42s1
dwc_ddrphy_apb_wr(0x900a9, 0x159); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b42s2
dwc_ddrphy_apb_wr(0x900aa, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b43s0
dwc_ddrphy_apb_wr(0x900ab, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b43s1
dwc_ddrphy_apb_wr(0x900ac, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b43s2
dwc_ddrphy_apb_wr(0x900ad, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b44s0
dwc_ddrphy_apb_wr(0x900ae, 0x3c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b44s1
dwc_ddrphy_apb_wr(0x900af, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b44s2
dwc_ddrphy_apb_wr(0x900b0, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b45s0
dwc_ddrphy_apb_wr(0x900b1, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b45s1
dwc_ddrphy_apb_wr(0x900b2, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b45s2
dwc_ddrphy_apb_wr(0x900b3, 0xc); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b46s0
dwc_ddrphy_apb_wr(0x900b4, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b46s1
dwc_ddrphy_apb_wr(0x900b5, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b46s2
dwc_ddrphy_apb_wr(0x900b6, 0x3); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b47s0
dwc_ddrphy_apb_wr(0x900b7, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b47s1
dwc_ddrphy_apb_wr(0x900b8, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b47s2
dwc_ddrphy_apb_wr(0x900b9, 0x7); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b48s0
dwc_ddrphy_apb_wr(0x900ba, 0x7c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b48s1
dwc_ddrphy_apb_wr(0x900bb, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b48s2
//// [phyinit_LoadPIECodeSections] Matched ANY enable_bits = 8, type = 0
dwc_ddrphy_apb_wr(0x900bc, 0x3a); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b49s0
dwc_ddrphy_apb_wr(0x900bd, 0x1e2); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b49s1
dwc_ddrphy_apb_wr(0x900be, 0x9); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b49s2
dwc_ddrphy_apb_wr(0x900bf, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b50s0
dwc_ddrphy_apb_wr(0x900c0, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b50s1
dwc_ddrphy_apb_wr(0x900c1, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b50s2
dwc_ddrphy_apb_wr(0x900c2, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b51s0
dwc_ddrphy_apb_wr(0x900c3, 0x8140); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b51s1
dwc_ddrphy_apb_wr(0x900c4, 0x10c); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b51s2
dwc_ddrphy_apb_wr(0x900c5, 0x10); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b52s0
dwc_ddrphy_apb_wr(0x900c6, 0x8138); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b52s1
dwc_ddrphy_apb_wr(0x900c7, 0x10c); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b52s2
//// [phyinit_LoadPIECodeSections] Matched ANY enable_bits = 1, type = 0
dwc_ddrphy_apb_wr(0x900c8, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b53s0
dwc_ddrphy_apb_wr(0x900c9, 0x400); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b53s1
dwc_ddrphy_apb_wr(0x900ca, 0x10e); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b53s2
dwc_ddrphy_apb_wr(0x900cb, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b54s0
dwc_ddrphy_apb_wr(0x900cc, 0x448); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b54s1
dwc_ddrphy_apb_wr(0x900cd, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b54s2
dwc_ddrphy_apb_wr(0x900ce, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b55s0
dwc_ddrphy_apb_wr(0x900cf, 0x7c8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b55s1
dwc_ddrphy_apb_wr(0x900d0, 0x101); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b55s2
dwc_ddrphy_apb_wr(0x900d1, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b56s0
dwc_ddrphy_apb_wr(0x900d2, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b56s1
dwc_ddrphy_apb_wr(0x900d3, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b56s2
dwc_ddrphy_apb_wr(0x900d4, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b57s0
dwc_ddrphy_apb_wr(0x900d5, 0x448); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b57s1
dwc_ddrphy_apb_wr(0x900d6, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b57s2
dwc_ddrphy_apb_wr(0x900d7, 0xf); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b58s0
dwc_ddrphy_apb_wr(0x900d8, 0x7c0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b58s1
dwc_ddrphy_apb_wr(0x900d9, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b58s2
dwc_ddrphy_apb_wr(0x900da, 0x7); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b59s0
dwc_ddrphy_apb_wr(0x900db, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b59s1
dwc_ddrphy_apb_wr(0x900dc, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b59s2
dwc_ddrphy_apb_wr(0x900dd, 0x47); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b60s0
dwc_ddrphy_apb_wr(0x900de, 0x630); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b60s1
dwc_ddrphy_apb_wr(0x900df, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b60s2
dwc_ddrphy_apb_wr(0x900e0, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b61s0
dwc_ddrphy_apb_wr(0x900e1, 0x618); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b61s1
dwc_ddrphy_apb_wr(0x900e2, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b61s2
dwc_ddrphy_apb_wr(0x900e3, 0x18); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b62s0
dwc_ddrphy_apb_wr(0x900e4, 0xe0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b62s1
dwc_ddrphy_apb_wr(0x900e5, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b62s2
dwc_ddrphy_apb_wr(0x900e6, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b63s0
dwc_ddrphy_apb_wr(0x900e7, 0x7c8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b63s1
dwc_ddrphy_apb_wr(0x900e8, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b63s2
dwc_ddrphy_apb_wr(0x900e9, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b64s0
dwc_ddrphy_apb_wr(0x900ea, 0x8140); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b64s1
dwc_ddrphy_apb_wr(0x900eb, 0x10c); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b64s2
dwc_ddrphy_apb_wr(0x900ec, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b65s0
dwc_ddrphy_apb_wr(0x900ed, 0x478); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b65s1
dwc_ddrphy_apb_wr(0x900ee, 0x109); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b65s2
dwc_ddrphy_apb_wr(0x900ef, 0x0); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b66s0
dwc_ddrphy_apb_wr(0x900f0, 0x1); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b66s1
dwc_ddrphy_apb_wr(0x900f1, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b66s2
dwc_ddrphy_apb_wr(0x900f2, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b67s0
dwc_ddrphy_apb_wr(0x900f3, 0x4); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b67s1
dwc_ddrphy_apb_wr(0x900f4, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b67s2
dwc_ddrphy_apb_wr(0x900f5, 0x8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b68s0
dwc_ddrphy_apb_wr(0x900f6, 0x7c8); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b68s1
dwc_ddrphy_apb_wr(0x900f7, 0x101); // DWC_DDRPHYA_INITENG0_base0_SequenceReg0b68s2
//// [phyinit_LoadPIECodeSections] Moving start address from 900f8 to 90006
dwc_ddrphy_apb_wr(0x90006, 0x0); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b0s0
dwc_ddrphy_apb_wr(0x90007, 0x0); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b0s1
dwc_ddrphy_apb_wr(0x90008, 0x8); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b0s2
dwc_ddrphy_apb_wr(0x90009, 0x0); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b1s0
dwc_ddrphy_apb_wr(0x9000a, 0x0); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b1s1
dwc_ddrphy_apb_wr(0x9000b, 0x0); // DWC_DDRPHYA_INITENG0_base0_PostSequenceReg0b1s2
//// [phyinit_LoadPIECodeSections] Moving start address from 9000c to d00e7
dwc_ddrphy_apb_wr(0xd00e7, 0x400); // DWC_DDRPHYA_APBONLY0_SequencerOverride
//// [phyinit_LoadPIECodeSections] End of dwc_ddrphy_phyinit_LoadPIECodeSections()
//seq0b_LoadPstateSeqProductionCode(): ---------------------------------------------------------------------------------------------------
//seq0b_LoadPstateSeqProductionCode(): Programming the 0B sequencer 0b0000 start vector registers with 0.
//seq0b_LoadPstateSeqProductionCode(): Programming the 0B sequencer 0b1111 start vector register with 56.
//seq0b_LoadPstateSeqProductionCode(): ---------------------------------------------------------------------------------------------------
dwc_ddrphy_apb_wr(0x90017, 0x0); // DWC_DDRPHYA_INITENG0_base0_StartVector0b0
dwc_ddrphy_apb_wr(0x90026, 0x38); // DWC_DDRPHYA_INITENG0_base0_StartVector0b15
dwc_ddrphy_apb_wr(0x9000c, 0x0); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag0
dwc_ddrphy_apb_wr(0x9000d, 0x173); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag1
dwc_ddrphy_apb_wr(0x9000e, 0x60); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag2
dwc_ddrphy_apb_wr(0x9000f, 0x6110); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag3
dwc_ddrphy_apb_wr(0x90010, 0x2152); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag4
dwc_ddrphy_apb_wr(0x90011, 0xdfbd); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag5
dwc_ddrphy_apb_wr(0x90012, 0xffff); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag6
dwc_ddrphy_apb_wr(0x90013, 0x6152); // DWC_DDRPHYA_INITENG0_base0_Seq0BDisableFlag7
//// [phyinit_I_loadPIEImage] Programming D4PowerControl::D4CATxDllLP to 0x1
//// [phyinit_I_loadPIEImage] Programming AcLcdlMasDis to 0xfff
dwc_ddrphy_apb_wr(0x2006d, 0x1); // DWC_DDRPHYA_MASTER0_base0_D4PowerControl
dwc_ddrphy_apb_wr(0x200e8, 0xfff); // DWC_DDRPHYA_MASTER0_base0_AcLcdlMasDis
//// [phyinit_I_loadPIEImage] Turn on calibration and hold idle until dfi_init_start is asserted sequence is triggered.
//// [phyinit_I_loadPIEImage] Programming CalZap to 0x1
//// [phyinit_I_loadPIEImage] Programming CalRate::CalRun to 0x1
//// [phyinit_I_loadPIEImage] Programming CalRate to 0x19
dwc_ddrphy_apb_wr(0x20089, 0x1); // DWC_DDRPHYA_MASTER0_base0_CalZap
dwc_ddrphy_apb_wr(0x20088, 0x19); // DWC_DDRPHYA_MASTER0_base0_CalRate
//// [phyinit_I_loadPIEImage] Programming ForceClkGaterEnables::ForcePubDxClkEnLow to 0x0
dwc_ddrphy_apb_wr(0x200a6, 0x0); // DWC_DDRPHYA_MASTER0_base0_ForceClkGaterEnables
//// Disabling Ucclk (PMU) and Hclk (training hardware)
dwc_ddrphy_apb_wr(0xc0080, 0x0); // DWC_DDRPHYA_DRTUB0_UcclkHclkEnables
//// Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1.
dwc_ddrphy_apb_wr(0xd0000, 0x1); // DWC_DDRPHYA_APBONLY0_MicroContMuxSel
//// [phyinit_userCustom_wait] Wait 40 DfiClks
//// [phyinit_I_loadPIEImage] End of dwc_ddrphy_phyinit_I_loadPIEImage()
//
//
////##############################################################
////
//// dwc_ddrphy_phyinit_userCustom_customPostTrain is a user-editable function.
////
//// The purpose of dwc_ddrphy_phyinit_userCustom_customPostTrain() is to override any
//// CSR values programmed by the training firmware or dwc_ddrphy_phyinit_progCsrSkipTrain()
//// This function is executed after training
////
//// IMPORTANT: in this function, user shall not override any values in userInputBasic and
//// userInputAdvanced data structures. Only CSR programming should be done in this function.
////
//// Sequence of Events in this function are:
//// 1. Enable APB access.
//// 2. Issue register writes
//// 3. Isolate APB access.
//
////##############################################################
//
dwc_ddrphy_phyinit_userCustom_customPostTrain();

//// [dwc_ddrphy_phyinit_userCustom_customPostTrain] End of dwc_ddrphy_phyinit_userCustom_customPostTrain()
//// [dwc_ddrphy_phyinit_userCustom_J_enterMissionMode] Start of dwc_ddrphy_phyinit_userCustom_J_enterMissionMode()
//
//
////##############################################################
////
//// 4.3.10(J) Initialize the PHY to Mission Mode through DFI Initialization
////
//// Initialize the PHY to mission mode as follows:
////
//// 1. Set the PHY input clocks to the desired frequency.
//// 2. Initialize the PHY to mission mode by performing DFI Initialization.
////    Please see the DFI specification for more information. See the DFI frequency bus encoding in section <XXX>.
//// Note: The PHY training firmware initializes the DRAM state. if skip
//// training is used, the DRAM state is not initialized.
////
////##############################################################
//
dwc_ddrphy_phyinit_userCustom_J_enterMissionMode();

//
//// [dwc_ddrphy_phyinit_userCustom_J_enterMissionMode] End of dwc_ddrphy_phyinit_userCustom_J_enterMissionMode()
// [dwc_ddrphy_phyinit_sequence] End of dwc_ddrphy_phyinit_sequence()
// [dwc_ddrphy_phyinit_main] End of dwc_ddrphy_phyinit_main()
