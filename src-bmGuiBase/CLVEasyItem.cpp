//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <string.h>
#include <Region.h>
#include <ClassInfo.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "CLVEasyItem.h"
#include "Colors.h"
#include "CLVColumn.h"
#include "ColumnListView.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** CLVEasyItem CLASS DEFINITION
//******************************************************************************************************
CLVEasyItem::CLVEasyItem(uint32 level, bool superitem, bool expanded, 
								 ColumnListView* lv)
:  CLVListItem( (lv && lv->fAvoidColPushing) ? 0 : level,
					 superitem,
					 expanded)
,  m_column_content( MIN( 4, lv->CountColumns()))
{
	fOwner = lv;
}


CLVEasyItem::~CLVEasyItem()
{
	int num_columns = m_column_content.CountItems();
	CLVColumn* column;
	for(int c = 0; c < num_columns; c++) {
		column = (CLVColumn*)fOwner->ColumnAt(c);
		if (column && column->Type() == CLV_COLTYPE_STATICTEXT)
			delete[] ((char*)m_column_content.ItemAt(c));
	}
}


void CLVEasyItem::PrepListsForSet(int column_index)
{
	int cur_num_columns = m_column_content.CountItems();
	bool delete_old = (cur_num_columns > column_index);
	while(cur_num_columns <= column_index)
	{
		m_column_content.AddItem(NULL);
		cur_num_columns++;
	}
	if(delete_old)
	{
		//Column content exists already so delete the old entries
		CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
		if (column && column->Type() == CLV_COLTYPE_STATICTEXT) {
			delete [] ((char*)m_column_content.ItemAt(column_index));
			((char**)m_column_content.Items())[column_index] = NULL;
		}
	}
}


void CLVEasyItem::SetColumnContent(int column_index, const char *text)
{
	PrepListsForSet(column_index);

	//Create the new entry
	if (text == NULL || text[0] == 0)
		((char**)m_column_content.Items())[column_index] = NULL;
	else
		((char**)m_column_content.Items())[column_index] = Strdup_new(text);
}


void CLVEasyItem::SetColumnContent(int column_index, 
											  const BmBitmapHandle *bitmap, 
											  int8 horizontal_offset)
{
	PrepListsForSet(column_index);

	//Create the new entry
	CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
	if (column) {
		column->BitmapOffset( horizontal_offset);
		if(bitmap == NULL)
			((char**)m_column_content.Items())[column_index] = NULL;
		else
			((const BmBitmapHandle**)m_column_content.Items())[column_index] = bitmap;
	}
}


const char* CLVEasyItem::GetColumnContentText(int column_index)
{
	CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
	if (column) {
		int32 type = column->Type();
		if (type == CLV_COLTYPE_STATICTEXT)
			return (char*)m_column_content.ItemAt(column_index);
		if (type == CLV_COLTYPE_USERTEXT)
			return GetUserText(column_index,column->Width());
	}
	return NULL;
}


const BmBitmapHandle* CLVEasyItem::GetColumnContentBitmap(int column_index)
{
	CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
	if (column) {
		int32 type = column->Type();
		if (type == CLV_COLTYPE_BITMAP)
		return (const BmBitmapHandle*)m_column_content.ItemAt(column_index);
	}
	return NULL;
}

bool CLVEasyItem::ColumnFitsText(int column_index, const char* text) const
{
	CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
	if (!column)
		return false;
	bool striped = ((ColumnListView*)fOwner)->StripedBackground();
	float offs = striped ? 5.0 : 2.0;
	BFont font;
	fOwner->GetFont(&font);
	if (Bold())
		font.SetFace(B_BOLD_FACE);
	float string_width = font.StringWidth(text);
	return offs + string_width <= column->Width();
}

struct CLVEasyItemDrawingContext : public CLVDrawingContext
{
	rgb_color backColor, tintedBackColor;
	BFont textFont, ownerFont;
	float hOffs, vOffs;
	bool needToRestoreOwnerFont, striped;
	ColumnListView* owner;
	
	virtual ~CLVEasyItemDrawingContext() {
		if (needToRestoreOwnerFont)
			owner->SetFont(&ownerFont);
	}
};

CLVDrawingContext* CLVEasyItem::CreateDrawingContext()
{
	return new CLVEasyItemDrawingContext;
}

void CLVEasyItem::SetupDrawingContext(CLVDrawingContext* drawingContext)
{
	CLVEasyItemDrawingContext* ctx
		= dynamic_cast<CLVEasyItemDrawingContext*>(drawingContext);
	if (ctx) {
		bool selected = IsSelected();
		ctx->striped = fOwner->StripedBackground();
		ctx->hOffs = ctx->striped ? 5.0 : 2.0;
		ctx->owner = fOwner;

		if(selected) {
			ctx->backColor = fOwner->ItemSelectColor();
			ctx->tintedBackColor = fOwner->ItemSelectColorTinted();
		} else {
			ctx->backColor = fOwner->LightColumnCol();
			ctx->tintedBackColor = fOwner->DarkColumnCol();
		}
		if (Highlight()) {
			const float highlight_tint = B_DARKEN_1_TINT;
			ctx->backColor = tint_color( ctx->backColor, highlight_tint);
			ctx->tintedBackColor 
				= tint_color( ctx->tintedBackColor, highlight_tint);
		}
		if (Bold()) {
			ctx->needToRestoreOwnerFont = true;
			fOwner->GetFont(&ctx->ownerFont);
			BFont boldfont(&ctx->ownerFont);
			boldfont.SetFace(B_BOLD_FACE);
			fOwner->SetFont(&boldfont);
			ctx->textFont = boldfont;
		} else {
			ctx->needToRestoreOwnerFont = false;
			fOwner->GetFont(&ctx->textFont);
		}
		font_height fontAttrs;
		ctx->textFont.GetHeight( &fontAttrs);
		float fontHeight = ceilf(fontAttrs.ascent) + ceilf(fontAttrs.descent);
		ctx->vOffs = ceilf(fontAttrs.ascent) + (Height()-fontHeight)/2.0f;
	}
	CLVListItem::SetupDrawingContext(drawingContext);
}

void CLVEasyItem::DrawColumn(BRect item_column_rect, int32 column_index,
									  CLVDrawingContext* drawingContext)
{
	CLVEasyItemDrawingContext* ctx
		= dynamic_cast<CLVEasyItemDrawingContext*>(drawingContext);
	if (!ctx)
		return;
	int32 index = fOwner->GetDisplayIndexForColumn( abs(column_index));
	if (column_index < 0)
		index = abs(column_index);
	if (ctx->striped && index % 2)
		fOwner->SetLowColor( ctx->tintedBackColor);
	else
		fOwner->SetLowColor( ctx->backColor);
	fOwner->FillRect( item_column_rect, B_SOLID_LOW);
	
	if(column_index < 0)
		return;

	CLVColumn* column = (CLVColumn*)fOwner->ColumnAt(column_index);
	if (!column)
		return;
	uint32 type = column->Type();
	bool right_justify = false;
	if (column->Flags() & CLV_RIGHT_JUSTIFIED)
		right_justify = true;

	BRegion Region;
	Region.Include(item_column_rect);
	fOwner->ConstrainClippingRegion(&Region);

	if (HighlightTop()) {
		BRect highlightRect( item_column_rect.left, 0,
									item_column_rect.right, 1);
		fOwner->SetHighColor( ui_color( B_CONTROL_HIGHLIGHT_COLOR));
		fOwner->FillRect( highlightRect);
	}

	if (HighlightBottom()) {
		BRect highlightRect( item_column_rect.left, 
									item_column_rect.bottom-1,
									item_column_rect.right, 
									item_column_rect.bottom);
		fOwner->SetHighColor( ui_color( B_CONTROL_HIGHLIGHT_COLOR));
		fOwner->FillRect( highlightRect);
	}

	if(type == CLV_COLTYPE_STATICTEXT || type == CLV_COLTYPE_USERTEXT)
	{
		const char* text = NULL;
		fOwner->SetHighColor( ui_color( B_DOCUMENT_TEXT_COLOR));
		if(type == CLV_COLTYPE_STATICTEXT)
			text = (const char*)m_column_content.ItemAt(column_index);
		else if(type == CLV_COLTYPE_USERTEXT)
			text = GetUserText(column_index,column->Width());

		if(text != NULL)
		{
			BPoint draw_point;
			if(!right_justify)
				draw_point.Set(item_column_rect.left+ctx->hOffs,
									item_column_rect.top+ctx->vOffs);
			else
			{
				float string_width = ctx->textFont.StringWidth(text);
				draw_point.Set(item_column_rect.right-ctx->hOffs-string_width,
									item_column_rect.top+ctx->vOffs);
			}				
			fOwner->DrawString(text,draw_point);
		}
	}
	else if(type == CLV_COLTYPE_BITMAP)
	{
		const BmBitmapHandle* bitmapHandle 
			= (const BmBitmapHandle*)m_column_content.ItemAt(column_index);
		const BBitmap* bitmap = bitmapHandle ? bitmapHandle->bitmap : NULL;
		if (bitmap != NULL) {
			BRect bounds = bitmap->Bounds();
			float horizontal_offset = column->BitmapOffset();
			if(!right_justify)
			{
				item_column_rect.left += horizontal_offset;
				item_column_rect.right = item_column_rect.left + (bounds.right-bounds.left);
			}
			else
			{
				item_column_rect.left = item_column_rect.right-horizontal_offset-(bounds.right-bounds.left);
				item_column_rect.right -= horizontal_offset;
			}
			item_column_rect.top += ceilf(((item_column_rect.bottom-item_column_rect.top)-(bounds.bottom-bounds.top))/2.0f);
			item_column_rect.bottom = item_column_rect.top + (bounds.bottom-bounds.top);
			fOwner->SetDrawingMode( B_OP_ALPHA);
			fOwner->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			fOwner->DrawBitmapAsync(bitmap,item_column_rect);
			fOwner->SetDrawingMode(B_OP_COPY);
		}
	}
	fOwner->ConstrainClippingRegion(NULL);
}


void CLVEasyItem::Update(BView *fOwner, const BFont *font)
{
	CLVListItem::Update(fOwner,font);
}


int CLVEasyItem::CompareItems(const CLVListItem *a_Item1, const CLVListItem *a_Item2, int32 KeyColumn,
										int32 col_flags)
{
	const CLVEasyItem* Item1 = cast_as(a_Item1,const CLVEasyItem);
	const CLVEasyItem* Item2 = cast_as(a_Item2,const CLVEasyItem);
	if (Item1 == NULL || Item2 == NULL)
		return 0;
	
	uint32 datatype = col_flags & CLV_COLDATAMASK;
	
	if (datatype == CLV_COLDATA_NUMBER) {
		int32 num1 = Item1->GetNumValueForColumn( KeyColumn);
		int32 num2 = Item2->GetNumValueForColumn( KeyColumn);

		return num1<num2 ? -1 : (num1>num2 ? 1 : 0);
	} else if (datatype == CLV_COLDATA_DATE) {
		time_t date1 = Item1->GetDateValueForColumn( KeyColumn);
		time_t date2 = Item2->GetDateValueForColumn( KeyColumn);

		return date1<date2 ? -1 : (date1>date2 ? 1 : 0);
	} else if (datatype == CLV_COLDATA_BIGTIME) {
		bigtime_t date1 = Item1->GetBigtimeValueForColumn( KeyColumn);
		bigtime_t date2 = Item2->GetBigtimeValueForColumn( KeyColumn);

		return date1<date2 ? -1 : (date1>date2 ? 1 : 0);
	} else {
		const char* text1 = NULL;
		const char* text2 = NULL;
		int32 type = col_flags & CLV_COLTYPE_MASK;

		if (type == CLV_COLTYPE_STATICTEXT)
			text1 = (const char*)Item1->m_column_content.ItemAt(KeyColumn);
		else if (type == CLV_COLTYPE_USERTEXT)
			text1 = Item1->GetUserText(KeyColumn,-1);

		if (type == CLV_COLTYPE_STATICTEXT)
			text2 = (const char*)Item2->m_column_content.ItemAt(KeyColumn);
		else if (type == CLV_COLTYPE_USERTEXT)
			text2 = Item2->GetUserText(KeyColumn,-1);

		if (!text1) {
			if (!text2)
				return 0;
			else
				return -1;
		}
		if (!text2)
			return 1;
		return strcasecmp(text1,text2);
	}
}

const char* CLVEasyItem::GetUserText(int32, float) const
{
	return NULL;
}

void CLVEasyItem::Highlight( bool b) 
{ 
	SetStyleFlag( CLV_STYLE_HIGHLIGHT, b); 
}

bool CLVEasyItem::Highlight( ) const 
{ 
	return (fItemFlags & CLV_STYLE_HIGHLIGHT) != 0; 
}

void CLVEasyItem::HighlightTop( bool b) 
{ 
	SetStyleFlag( CLV_STYLE_HIGHLIGHT_TOP, b); 
}

bool CLVEasyItem::HighlightTop( ) const 
{ 
	return (fItemFlags & CLV_STYLE_HIGHLIGHT_TOP) != 0; 
}

void CLVEasyItem::HighlightBottom( bool b) 
{ 
	SetStyleFlag( CLV_STYLE_HIGHLIGHT_BOTTOM, b); 
}

bool CLVEasyItem::HighlightBottom( ) const
{ 
	return (fItemFlags & CLV_STYLE_HIGHLIGHT_BOTTOM) != 0; 
}

void CLVEasyItem::Bold( bool b) 
{ 
	SetStyleFlag( CLV_STYLE_BOLD, b); 
}

bool CLVEasyItem::Bold( ) const 
{ 
	return (fItemFlags & CLV_STYLE_BOLD) != 0; 
}
