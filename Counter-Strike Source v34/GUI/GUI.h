#pragma once
#ifndef __GUI_H_
#define __GUI_H_

class GUI
{
public:
	bool ShouldDisableInput( void );
	void SetupStyle( void );
	void DrawImGui( void );

private:
	void RenderAimbotTab( void );
	void RenderVisualsTab( void );
	void RenderMiscTab( void );
	void RenderPlayerListTab( void );
	void RenderConfigsTab( void );
};

extern GUI g_GUI;

#endif // __GUI_H_