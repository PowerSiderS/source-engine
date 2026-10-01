#ifndef OPTIONS_SUB_INVENTORY_H
#define OPTIONS_SUB_INVENTORY_H
#include <vgui_controls/PropertyPage.h>
#include "tier1/utlvector.h"
namespace vgui { class Button; class ImagePanel; class Label; class ComboBox; class ScrollBar; }
class COptionsSubInventory : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubInventory, vgui::PropertyPage );
public:
	COptionsSubInventory( vgui::Panel *parent );
	virtual void PerformLayout();
	virtual void PaintBackground() {} // The inventory frame owns its translucent backdrop.
	virtual void OnCommand( const char *command );
	MESSAGE_FUNC( OnCategoryChanged, "TextChanged" );
	MESSAGE_FUNC_INT( OnScroll, "ScrollBarSliderMoved", position );
private:
	struct Card { int id; char name[96], category[32], icon[192]; vgui::Button *button; vgui::ImagePanel *image; };
	CUtlVector<Card> m_Cards;
	vgui::Panel *m_Grid;
	vgui::ComboBox *m_Category;
	vgui::ScrollBar *m_Scroll;
	vgui::ImagePanel *m_Preview;
	vgui::Label *m_Name;
	vgui::Label *m_Status;
	vgui::Button *m_Equip;
	int m_Selected;
};
#endif
