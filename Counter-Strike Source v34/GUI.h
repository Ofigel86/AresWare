#pragma once
#ifndef __GUI_H_
#define __GUI_H_

class AimbotSettings;

class GUI
{
public:
	bool ShouldDisableInput( void );
	void SetupStyle( void );
	void DrawImGui( void );

private:
	void RenderAimbotTab( void ); // legacy wrapper, now redirects to Rage
	void RenderLegitTab( void );
	void RenderRageTab( void );
	void RenderAimbotCommon( AimbotSettings& profile, bool bIsLegit );
	void RenderVisualsTab( void );
	void RenderMiscTab( void );
	void RenderPlayerListTab( void );
	void RenderConfigsTab( void );
};

extern GUI g_GUI;

#endif // __GUI_H_