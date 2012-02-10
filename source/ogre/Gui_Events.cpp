#include "pch.h"
#include "common/Defines.h"
#include "../vdrift/pathmanager.h"
#include "../vdrift/game.h"
#include "../road/Road.h"
#include "OgreGame.h"
#include "FollowCamera.h"
#include "SplitScreen.h"
#include "common/Gui_Def.h"
#include "common/RenderConst.h"
#include "../network/masterclient.hpp"
#include "../network/gameclient.hpp"

#include <MyGUI_PointerManager.h>
#include <OIS/OIS.h>
#include "../oisb/OISB.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <OgreRoot.h>
#include <OgreTerrain.h>
#include <OgreMaterialManager.h>
#include <OgreOverlay.h>
#include <OgreRenderWindow.h>
using namespace std;
using namespace Ogre;
using namespace MyGUI;


///  Gui Events

#define ChkEv(var)  \
	pSet->var = !pSet->var;  if (wp) {  \
	ButtonPtr chk = wp->castType<MyGUI::Button>(); \
	chk->setStateSelected(pSet->var);  }


//  [Setup]
//    [Car]
void App::chkAbs(WP wp){		ChkEv(abs);		if (pGame)  pGame->ProcessNewSettings();	}
void App::chkTcs(WP wp){		ChkEv(tcs);		if (pGame)  pGame->ProcessNewSettings();	}

void App::chkGear(WP wp){		ChkEv(autoshift);	if (pGame)  pGame->ProcessNewSettings();	}
void App::chkRear(WP wp){		ChkEv(autorear);	if (pGame)  pGame->ProcessNewSettings();	}
void App::chkRearInv(WP wp){	ChkEv(rear_inv);	if (pGame)  pGame->ProcessNewSettings();	}
//    [Game]
void App::chkVegetCollis(WP wp){	ChkEv(gui.collis_veget);	}
void App::chkCarCollis(WP wp){		ChkEv(gui.collis_cars);		}

//  boost, flip
void App::comboBoost(CMB)
{
	pSet->gui.boost_type = val;  ShowHUD();
}
void App::comboFlip(CMB)
{
	pSet->gui.flip_type = val;
}
	
void App::btnNumPlayers(WP wp)
{
	if      (wp->getName() == "btnPlayers1")  pSet->gui.local_players = 1;
	else if (wp->getName() == "btnPlayers2")  pSet->gui.local_players = 2;
	else if (wp->getName() == "btnPlayers3")  pSet->gui.local_players = 3;
	else if (wp->getName() == "btnPlayers4")  pSet->gui.local_players = 4;
	if (valLocPlayers)  valLocPlayers->setCaption(toStr(pSet->gui.local_players));
}
void App::chkSplitVert(WP wp)
{
	ChkEv(split_vertically); 
}

void App::slNumLaps(SL)
{
	int v = 20.f * val/res + 1;  if (bGI)  pSet->gui.num_laps = v;
	if (valNumLaps){  valNumLaps->setCaption(toStr(v));  }
}

void App::tabPlayer(TabPtr wp, size_t id)
{
	iCurCar = id;
	//  update gui for this car (color h,s,v, name, img)
	size_t i = carList->findItemIndexWith(pSet->gui.car[iCurCar]);
	if (i != ITEM_NONE)
	{	carList->setIndexSelected(i);
		listCarChng(carList, i);
	}
	UpdCarClrSld(false);  // no car color change
}

//  car color
void App::slCarClrH(SL)
{
	Real v = val/res;  if (bGI)  pSet->gui.car_hue[iCurCar] = v;
	if (valCarClrH){	valCarClrH->setCaption(fToStr(v,2,4));  }
	if (iCurCar < carModels.size() && bUpdCarClr && bGI)
		carModels[iCurCar]->ChangeClr(iCurCar);
}
void App::slCarClrS(SL)
{
	Real v = val/res;  if (bGI)  pSet->gui.car_sat[iCurCar] = v;
	if (valCarClrS){	valCarClrS->setCaption(fToStr(v,2,4));  }
	if (iCurCar < carModels.size() && bUpdCarClr && bGI)
		carModels[iCurCar]->ChangeClr(iCurCar);
}
void App::slCarClrV(SL)
{
	Real v = val/res;  if (bGI)  pSet->gui.car_val[iCurCar] = v;
	if (valCarClrV){	valCarClrV->setCaption(fToStr(v,2,4));  }
	if (iCurCar < carModels.size() && bUpdCarClr && bGI)
		carModels[iCurCar]->ChangeClr(iCurCar);
}

void App::imgBtnCarClr(WP img)
{
	pSet->gui.car_hue[iCurCar] = s2r(img->getUserString("h"));
	pSet->gui.car_sat[iCurCar] = s2r(img->getUserString("s"));
	pSet->gui.car_val[iCurCar] = s2r(img->getUserString("v"));
	UpdCarClrSld();
}
void App::btnCarClrRandom(WP)
{
	pSet->gui.car_hue[iCurCar] = Math::UnitRandom();
	pSet->gui.car_sat[iCurCar] = Math::UnitRandom();
	pSet->gui.car_val[iCurCar] = Math::UnitRandom();
	UpdCarClrSld();
}


//  [Graphics]
//---------------------------------------------------------------------

//  particles/trails
void App::slParticles(SL)
{
	Real v = 4.f * powf(val/res, 2.f);  if (bGI)  pSet->particles_len = v;
	if (valParticles){	valParticles->setCaption(fToStr(v,2,4));  }
}
void App::slTrails(SL)
{
	Real v = 4.f * powf(val/res, 2.f);  if (bGI)  pSet->trails_len = v;
	if (valTrails){		valTrails->setCaption(fToStr(v,2,4));  }
}

//  reflect
void App::slReflSkip(SL)
{
	int v = 1000.f * powf(val/res, 2.f);	if (bGI)  pSet->refl_skip = v;
	if (valReflSkip)  valReflSkip->setCaption(toStr(v));
}
void App::slReflSize(SL)
{
	int v = std::max( 0.0f, std::min((float) ciShadowNumSizes-1, ciShadowNumSizes * val/res));
	if (bGI)  pSet->refl_size = v;
	if (valReflSize)  valReflSize->setCaption(toStr(ciShadowSizesA[v]));
}
void App::slReflFaces(SL)
{
	if (bGI)  pSet->refl_faces = val;
	if (valReflFaces)  valReflFaces->setCaption(toStr(val));
}
void App::slReflDist(SL)
{
	float v = 20.f + 1480.f * powf(val/res, 2.f);	if (bGI)  pSet->refl_dist = v;
	if (valReflDist){	valReflDist->setCaption(fToStr(v,0,4)+" m");  }
	
	recreateReflections();
}
void App::slReflMode(SL)
{
	std::string old = pSet->refl_mode;
	
	if (val == 0)  pSet->refl_mode = "static";  //enums..
	if (val == 1)  pSet->refl_mode = "single";
	if (val == 2)  pSet->refl_mode = "full";
	
	if (pSet->refl_mode != old)
		recreateReflections();
		
	if (valReflMode)
	{
		valReflMode->setCaption( TR("#{ReflMode_" + pSet->refl_mode + "}") );
		if (pSet->refl_mode == "static")  valReflMode->setTextColour(MyGUI::Colour(0.0, 1.0, 0.0)); 
		else if (pSet->refl_mode == "single")  valReflMode->setTextColour(MyGUI::Colour(1.0, 0.5, 0.0));
		else if (pSet->refl_mode == "full")  valReflMode->setTextColour(MyGUI::Colour(1.0, 0.0, 0.0));
	}
}
void App::recreateReflections()
{
	for (std::vector<CarModel*>::iterator it = carModels.begin(); it!=carModels.end(); it++)
	{	
		delete (*it)->pReflect;
		(*it)->CreateReflection();
	}
}


//  [View] size
void App::slSizeGaug(SL)
{
	float v = 0.1f + 0.15f * val/res;	if (bGI)  {  pSet->size_gauges = v;  SizeHUD(true);  }
	if (valSizeGaug){	valSizeGaug->setCaption(fToStr(v,3,4));  }
}
void App::slSizeArrow(SL)
{
	float v = val/res;	if (bGI)  {  pSet->size_arrow = v;  }
	if (valSizeArrow){	valSizeArrow->setCaption(fToStr(v,3,4));  }
	if (arrowNode) arrowRotNode->setScale(v/2.f, v/2.f, v/2.f);
}

void App::slCountdownTime(SL)
{
	float v = val * 0.5f;	if (bGI)  {  pSet->gui.pre_time = v;  }
	if (valCountdownTime){	valCountdownTime->setCaption(fToStr(v,1,4));  }
}

//  minimap
void App::slSizeMinimap(SL)
{
	float v = 0.05f + 0.25f * val/res;	if (bGI)  {  pSet->size_minimap = v;  SizeHUD(true);  }
	if (valSizeMinimap){	valSizeMinimap->setCaption(fToStr(v,3,4));  }
}
void App::slZoomMinimap(SL)
{
	float v = 1.f + 9.f * powf(val/res, 2.f);	if (bGI)  {  pSet->zoom_minimap = v;  SizeHUD(true);  }
	if (valZoomMinimap){	valZoomMinimap->setCaption(fToStr(v,3,4));  }
}


//  [Sound]
void App::slVolMaster(SL)
{
	Real v = 1.6f * val/res;	if (bGI)  {  pSet->vol_master = v;  pGame->ProcessNewSettings();  }
	if (valVolMaster){  valVolMaster->setCaption(fToStr(v,2,4));  }
}
void App::slVolEngine(SL)
{
	Real v = 1.4f * val/res;	if (bGI)  pSet->vol_engine = v;
	if (valVolEngine){  valVolEngine->setCaption(fToStr(v,2,4));  }
}
void App::slVolTires(SL)
{
	Real v = 1.4f * val/res;	if (bGI)  pSet->vol_tires = v;
	if (valVolTires){  valVolTires->setCaption(fToStr(v,2,4));  }
}
void App::slVolEnv(SL)
{
	Real v = 1.4f * val/res;	if (bGI)  pSet->vol_env = v;
	if (valVolEnv){  valVolEnv->setCaption(fToStr(v,2,4));  }
}


//  [Game] 	. . . . . . . . . . . . . . . . . . . .    --- lists ----    . . . . . . . . . . . . . . . . . . . .

//  car
void App::listCarChng(List* li, size_t pos)
{
	size_t i = li->getIndexSelected();  if (i==ITEM_NONE)  return;
	const UString& sl = li->getItemNameAt(i);	sListCar = sl;

	if (imgCar)  imgCar->setImageTexture(sListCar+".jpg");
	if (mClient) mClient->updatePlayerInfo(pSet->nickname, sListCar);
}
void App::btnChgCar(WP)
{
	if (valCar){  valCar->setCaption(TR("#{Car}: ") + sListCar);	pSet->gui.car[iCurCar] = sListCar;  }
}

//  track
void App::btnChgTrack(WP)
{
	pSet->gui.track = sListTrack;
	pSet->gui.track_user = bListTrackU;
	if (valTrk[0])  valTrk[0]->setCaption(TR("#{Track}: ") + sListTrack);

	if (mMasterClient) {
		uploadGameInfo();
		updateGameInfoGUI();
	}
}

//  new game
void App::btnNewGame(WP)
{
	if (mWndGame->getVisible() && mWndTabsGame->getIndexSelected() < 5)
		pSet->gui.champ_num = -1;  /// champ, back to single race
	
	NewGame();  isFocGui = false;  // off gui
	if (mWndOpts)  mWndOpts->setVisible(isFocGui);
	if (mWndRpl)  mWndRpl->setVisible(false);//
	if (bnQuit)  bnQuit->setVisible(isFocGui);
	
	updMouse();
	
	mToolTip->setVisible(false);
}
void App::btnNewGameStart(WP wp)
{
	btnChgTrack(wp);
	btnNewGame(wp);
}


//  [View]  . . . . . . . . . . . . . . . . . . . .    ---- checks ----    . . . . . . . . . . . . . . . . . . . .

void App::chkDigits(WP wp){ 		ChkEv(show_digits); ShowHUD();   }

void App::chkReverse(WP wp){		ChkEv(gui.trackreverse);	ReadTrkStats();  }

void App::chkParticles(WP wp)
{		
	ChkEv(particles);
	for (std::vector<CarModel*>::iterator it=carModels.begin(); it!=carModels.end(); it++)
	//? if ((*it)->eType != CarModel::CT_GHOST)
		(*it)->UpdParsTrails();
}
void App::chkTrails(WP wp)
{			
	ChkEv(trails);		
	for (std::vector<CarModel*>::iterator it=carModels.begin(); it!=carModels.end(); it++)
		(*it)->UpdParsTrails();
}
void App::chkFps(WP wp){			ChkEv(show_fps);	if (pSet->show_fps)  mFpsOverlay->show();  else  mFpsOverlay->hide();	}

void App::chkGauges(WP wp){			ChkEv(show_gauges);	ShowHUD();	}
void App::chkArrow(WP wp){			ChkEv(check_arrow); if (arrowRotNode) arrowRotNode->setVisible(pSet->check_arrow);  }
void App::chkMinimap(WP wp){		ChkEv(trackmap);	if (ndMap)  ndMap->setVisible(pSet->trackmap);	}
void App::chkMiniZoom(WP wp){		ChkEv(mini_zoomed);		}
void App::chkMiniRot(WP wp){		ChkEv(mini_rotated);	}
void App::chkMiniTer(WP wp){		ChkEv(mini_terrain);	UpdMiniTer();  }
void App::chkTimes(WP wp){			ChkEv(show_times);	ShowHUD();	}
void App::chkOpponents(WP wp){		ChkEv(show_opponents);	ShowHUD();	}

//void App::chkRacingLine(WP wp){		ChkEv(racingline);	if (ndLine)  ndLine->setVisible(pSet->racingline);	}
void App::chkCamInfo(WP wp){		ChkEv(show_cam);	ShowHUD();	}

void App::chkCarDbgBars(WP wp){		ChkEv(car_dbgbars);	ShowHUD();	}
void App::chkCarDbgTxt(WP wp){		ChkEv(car_dbgtxt);	ShowHUD();	}
void App::chkBltDebug(WP wp){		ChkEv(bltDebug);	}
void App::chkBltProfilerTxt(WP wp){	ChkEv(bltProfilerTxt);	}

void App::radKmh(WP wp){	bRkmh->setStateSelected(true);  bRmph->setStateSelected(false);  pSet->show_mph = false;  ShowHUD();  }
void App::radMph(WP wp){	bRkmh->setStateSelected(false);  bRmph->setStateSelected(true);  pSet->show_mph = true;   ShowHUD();  }

//  Startup
void App::chkOgreDialog(WP wp){		ChkEv(ogre_dialog);	}
void App::chkAutoStart(WP wp){		ChkEv(autostart);	}
void App::chkEscQuits(WP wp){		ChkEv(escquit);		}
void App::chkBltLines(WP wp){		ChkEv(bltLines);	}

void App::chkLoadPics(WP wp){		ChkEv(loadingbackground);	}


//  [Video]  . . . . . . . . . . . . . . . . . . . .    ---- ------ ----    . . . . . . . . . . . . . . . . . . . .

void App::chkVidEffects(WP wp)
{
	ChkEv(all_effects);  recreateCompositor();  //refreshCompositor();
}
void App::chkVidBloom(WP wp)
{		
	ChkEv(bloom);  refreshCompositor();
}
void App::chkVidHDR(WP wp)
{			
	ChkEv(hdr);  refreshCompositor();
}
void App::chkVidBlur(WP wp)
{		
	ChkEv(motionblur);  refreshCompositor();
}
void App::chkVidSSAA(WP wp)
{
	ChkEv(ssaa);  refreshCompositor();
}
void App::chkVidSSAO(WP wp)
{		
	ChkEv(ssao);  refreshCompositor();
}
void App::chkVidSoftParticles(WP wp)
{		
	ChkEv(softparticles);  refreshCompositor();
}
void App::chkVidGodRays(WP wp)
{		
	ChkEv(godrays);  refreshCompositor();
}
void App::slBloomInt(SL)
{
	Real v = val/res;  if (bGI)  pSet->bloomintensity = v;
	if (valBloomInt){	valBloomInt->setCaption(fToStr(v,2,4));  }
	if (bGI)  refreshCompositor();
}
void App::slBloomOrig(SL)
{
	Real v = val/res;  if (bGI)  pSet->bloomorig = v;
	if (valBloomOrig){	valBloomOrig->setCaption(fToStr(v,2,4));  }
	if (bGI)  refreshCompositor();
}
void App::slBlurIntens(SL)
{
	Real v = val/res;  if (bGI)  pSet->motionblurintensity = v;
	if (valBlurIntens){	valBlurIntens->setCaption(fToStr(v,2,4));  }
	// if (bGI)  refreshCompositor();   // intensity is set every frame in UpdateHUD
}



//-----------------------------------------------------------------------------------------------------------
//  Key pressed
//-----------------------------------------------------------------------------------------------------------

// util
bool App::actionIsActive(std::string name, std::string pressed)
{
	std::string actionKey = GetInputName(mOISBsys->lookupAction("General/" + name)->mBindings[0]->mBindables[0].second->getBindableName());
	boost::to_lower(actionKey);
	boost::to_lower(pressed);
	return actionKey == pressed;
}

bool App::keyPressed( const OIS::KeyEvent &arg )
{
	// update all keystates
	OISB::System::getSingleton().process(0.001/*?0*/);
	
	// action key == pressed key
	#define action(s)  actionIsActive(s, mKeyboard->getAsString(arg.key))

if (!bAssignKey)
{
	//  change gui tabs
	if (isFocGui && !pSet->isMain)
	{
		MyGUI::TabPtr tab = 0;
		switch (pSet->inMenu)
		{
			case WND_Game:  case WND_Champ:		tab = mWndTabsGame;  break;
			case WND_Options:  tab = mWndTabsOpts;  break;
		}
		if (tab)
		{	int num = tab->getItemCount()-1, i = 0;
			if (action("PrevTab")) {		i = tab->getIndexSelected();  if (i==1)  i = num;  else  --i;
				tab->setIndexSelected(i);  MenuTabChg(tab,i);  return true;  }
			else if (action("NextTab")) {	i = tab->getIndexSelected();  if (i==num)  i = 1;  else  ++i;
				tab->setIndexSelected(i);  MenuTabChg(tab,i);  return true;  }
		}
	}
	
	//  gui on/off
	if (action("ShowOptions"))
	{	if (mWndChampEnd->getVisible())  mWndChampEnd->setVisible(false);  // hide champs end
		toggleGui(true);  return false;  }

	//  new game - reload
	if (action("RestartGame"))
	{	NewGame();  return false;	}

	//  new game - fast (same track & cars)
	if (action("ResetGame"))
	{
		for (int c=0; c < carModels.size(); ++c)
		{
			if (carModels[c]->pCar)  carModels[c]->pCar->ResetPos(true);
			if (carModels[c]->fCam)  carModels[c]->fCam->first = true;
			carModels[c]->ResetChecks();
			carModels[c]->iWonPlace = 0;
		}
		pGame->timer.Reset(0);
		pGame->timer.pretime = mClient ? 2.0f : pSet->game.pre_time;  // same for all multi players
		carIdWin = 1;  //
		ghost.Clear(); //
	}
	
	using namespace OIS;


	//  main menu keys
	if (pSet->isMain)
	{
		switch (arg.key)
		{
		case KC_UP:  case KC_NUMPAD8:
			pSet->inMenu = (pSet->inMenu-1+WND_ALL)%WND_ALL;
			toggleGui(false);  return true;

		case KC_DOWN:  case KC_NUMPAD2:
			pSet->inMenu = (pSet->inMenu+1)%WND_ALL;
			toggleGui(false);  return true;

		case KC_RETURN:
			pSet->isMain = false;
			toggleGui(false);  return true;
		}
	}

	//  esc
	if (arg.key == KC_ESCAPE)
	{
		if (pSet->escquit)
			mShutDown = true;	// quit
		else
			if (mWndChampStage->getVisible())  ///  close champ wnds
				btnChampStageStart(0);
			else
				toggleGui(true);	// gui on/off
		return true;
	}

	//  shortcut keys for gui access (alt-T,H,S,G,V,.. )
	if (alt)
		switch (arg.key)
		{
			case KC_T:	GuiShortcut(WND_Game, 1);	return true;  // Track
			case KC_C:	GuiShortcut(WND_Game, 2);	return true;  // Car
			case KC_H:	GuiShortcut(WND_Champ, 4);	return true;  // Champs

			case KC_R:	GuiShortcut(WND_Replays, 1);	return true;  // [Replays]

			case KC_S:	GuiShortcut(WND_Options, 1);	return true;  // Screen
			case KC_E:	GuiShortcut(WND_Options, 1);	return true;  // -Effects
			case KC_G:	GuiShortcut(WND_Options, 2);	return true;  // Graphics

			case KC_V:	GuiShortcut(WND_Options, 3);	return true;  // View
			case KC_O:	GuiShortcut(WND_Options, 3);	return true;  // -Other
			case KC_I:	GuiShortcut(WND_Options, 4);	return true;  // Input
		}

	//  not main menus
	//if (/*&& !pSet->isMain*/)
	{
		Widget* wf = MyGUI::InputManager::getInstance().getKeyFocusWidget();
		bool edFoc = wf && wf->getTypeName() == "EditBox";
		//if (wf)  LogO(wf->getTypeName()+" " +toStr(edFoc));
		switch (arg.key)
		{
			case KC_BACK:
				if (mWndChampStage->getVisible())
				{	btnChampStageBack(0);  return true;  }
				if (pSet->isMain)  break;
				if (isFocGui)
				{	if (edFoc)  break;
					pSet->isMain = true;  toggleGui(false);  }
				else
					if (mWndRpl && !isFocGui)	bRplWnd = !bRplWnd;  // replay controls
				return true;

			case KC_P:		// replay play/pause
				if (bRplPlay && !isFocGui)
				{	bRplPause = !bRplPause;  UpdRplPlayBtn();
					return true;  }
				break;
				
			case KC_F:		// focus on find edit
				if (ctrl && edFind && isFocGui &&
					!pSet->isMain && pSet->inMenu == WND_Game && mWndTabsGame->getIndexSelected() == 1)
				{
					MyGUI::InputManager::getInstance().resetKeyFocusWidget();
					MyGUI::InputManager::getInstance().setKeyFocusWidget(edFind);
					return true;  }
				break;
				

			case KC_F9:		// car debug text/bars
				if (shift)	{	WP wp = chDbgT;  ChkEv(car_dbgtxt);  ShowHUD();  }
				else		{	WP wp = chDbgB;  ChkEv(car_dbgbars);   ShowHUD();  }
				return true;

			case KC_F11:	//  fps
			if (!shift)
			{	WP wp = chFps;  ChkEv(show_fps); 
				if (pSet->show_fps)  mFpsOverlay->show();  else  mFpsOverlay->hide();
				return false;
			}	break;

			case KC_F10:	//  blt debug, txt
			if (shift)
			{	WP wp = chBltTxt;  ChkEv(bltProfilerTxt);  return false;  }
			else if (ctrl)
			{	WP wp = chBlt;  ChkEv(bltDebug);  return false;  }
				break;

			case KC_F7:		// Times
			if (shift)
			{	WP wp = chOpponents;  ChkEv(show_opponents);  ShowHUD();  }
			else
			{	WP wp = chTimes;  ChkEv(show_times);  ShowHUD();  }
				return false;
				
			case KC_F8:		// Minimap
			{	WP wp = chMinimp;  ChkEv(trackmap);  if (ndMap)  ndMap->setVisible(pSet->trackmap);
			}	return false;
			
			case KC_F5:		//  new game
			{	NewGame();  return false;
			}	break;


			case KC_RETURN:		///  close champ wnds
			if (mWndChampStage->getVisible())
				btnChampStageStart(0);
			else			//  chng trk/car + new game  after up/dn
			if (isFocGui && !pSet->isMain)
				switch (pSet->inMenu)
				{
				case WND_Replays:
					btnRplLoad(0);  break;
				case WND_Game:  case WND_Champ:
				{	switch (mWndTabsGame->getIndexSelected())
					{
					case 1:
						btnChgTrack(0);
						btnNewGame(0);  break;
					case 2:
						btnChgCar(0);
						btnNewGame(0);  break;
					case 3:
						chatSendMsg();  break;
					case 5:
						btnChampStart(0);  break;
				}	break;
			}	}
			return false;
		}
	}
}
	InputBind(arg.key);
	
	if (!BaseApp::keyPressed(arg))
		return true;

	return true;
}


//  Menu
//---------------------------------------------------------------------

void App::toggleGui(bool toggle)
{
	if (toggle)
		isFocGui = !isFocGui;

	bool notMain = isFocGui && !pSet->isMain;
	if (mWndMain)	mWndMain->setVisible(isFocGui && pSet->isMain);
	if (mWndReplays) mWndReplays->setVisible(notMain && pSet->inMenu == WND_Replays);
	if (mWndHelp)	mWndHelp->setVisible(notMain && pSet->inMenu == WND_Help);
	if (mWndOpts)	mWndOpts->setVisible(notMain && pSet->inMenu == WND_Options);
	
	//  fill Readme editbox from file
	static bool first = true;
	if (mWndHelp && mWndHelp->getVisible() && first)
	{
		first = false;
		EditBox* edit = mGUI->findWidget<EditBox>("Readme");
		if (edit)
		{	std::string path = PATHMANAGER::GetDataPath()+"/../Readme.txt", text;
			std::ifstream fi(path.c_str());
			while (!fi.eof())
			{
				char buf[4*4096];
				fi.read(buf,sizeof(buf));
				text += buf;
			}
			edit->setCaption(UString(text));
			edit->setVScrollPosition(0);
	}	}

	///  update track tab, for champs wnd
	bool game = pSet->inMenu == WND_Game, champ = pSet->inMenu == WND_Champ, gc = game || champ;
	if (mWndGame)
	{	mWndGame->setVisible(notMain  && gc);
		if (mWndGame->getVisible())
			mWndGame->setCaption(champ ? TR("#{Championship}") : TR("#{SingleRace}"));
	}
	if (notMain && gc)  // show hide champs,stages
	{
		size_t id = mWndTabsGame->getIndexSelected();
		mWndTabsGame->setButtonWidthAt(1,champ ? 1 :-1);  if (id == 1 && champ)  mWndTabsGame->setIndexSelected(5);
		mWndTabsGame->setButtonWidthAt(4,champ ? 1 :-1);  if (id == 4 && champ)  mWndTabsGame->setIndexSelected(5);
		mWndTabsGame->setButtonWidthAt(5,champ ?-1 : 1);  if (id == 5 && !champ)  mWndTabsGame->setIndexSelected(1);
		mWndTabsGame->setButtonWidthAt(6,champ ?-1 : 1);  if (id == 6 && !champ)  mWndTabsGame->setIndexSelected(1);
		mWndTabsGame->setButtonWidthAt(7,champ ?-1 : 1);  if (id == 7 && !champ)  mWndTabsGame->setIndexSelected(1);
	}

	if (bnQuit)  bnQuit->setVisible(isFocGui);
	updMouse();
	if (!isFocGui)  mToolTip->setVisible(false);

	for (int i=0; i < WND_ALL; ++i)
		mWndMainPanels[i]->setVisible(pSet->inMenu == i);
}

void App::MainMenuBtn(MyGUI::WidgetPtr wp)
{
	for (int i=0; i < WND_ALL; ++i)
		if (wp == mWndMainBtns[i])
		{
			pSet->isMain = false;
			pSet->inMenu = i;
			toggleGui(false);
			return;
		}
}

void App::MenuTabChg(MyGUI::TabPtr tab, size_t id)
{
	if (id != 0)  return;
	tab->setIndexSelected(1);  // dont switch to 0
	pSet->isMain = true;
	toggleGui(false);  // back to main
}

void App::GuiShortcut(WND_Types wnd, int tab)
{
	isFocGui = true;
	pSet->isMain = false;  pSet->inMenu = wnd;
	switch (wnd)
	{	case WND_Champ:
		case WND_Game:		mWndTabsGame->setIndexSelected(tab);  break;
		//case WND_Replays:	mWndTabs->setIndexSelected(tab);  break;
		case WND_Options:	mWndTabsOpts->setIndexSelected(tab);  break;
	}
	toggleGui(false);
}
