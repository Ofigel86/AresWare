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
	public: void RenderAimbotTab( void );
	public: void RenderLegitTab( void );
	public: void RenderVisualsTab( void );
	public: void RenderMiscTab( void );
	public: void RenderPlayerListTab( void );
	public: void RenderConfigsTab( void );
	public: void RenderScriptsTab( void );
};

extern GUI g_GUI;

#endif // __GUI_H_