#include "ccextractor.h"

/* Portions by Daniel Kristjansson, extracted from MythTV's source */

// #define DEBUG_708_PACKETS   // Already working. 

unsigned char current_packet[MAX_708_PACKET_LENGTH]; // Length according to EIA-708B, part 5
int current_packet_length=0;
int last_seq=-1; // -1 -> No last sequence yet

const char *COMMANDS_C0[32]=
{
    "NUL", // 0 = NUL
    NULL,  // 1 = Reserved
    NULL,  // 2 = Reserved
    "ETX", // 3 = ETX
    NULL,  // 4 = Reserved
    NULL,  // 5 = Reserved
    NULL,  // 6 = Reserved
    NULL,  // 7 = Reserved
    "BS",  // 8 = Backspace
    NULL,  // 9 = Reserved
    NULL,  // A = Reserved
    NULL,  // B = Reserved    
    "FF",  // C = FF
    "CR",  // D = CR
    "HCR", // E = HCR
    NULL,  // F = Reserved    
    "EXT1",// 0x10 = EXT1,
    NULL,  // 0x11 = Reserved        
    NULL,  // 0x12 = Reserved        
    NULL,  // 0x13 = Reserved        
    NULL,  // 0x14 = Reserved        
    NULL,  // 0x15 = Reserved        
    NULL,  // 0x16 = Reserved
    NULL,  // 0x17 = Reserved                    
    "P16", // 0x18 = P16
    NULL,  // 0x19 = Reserved                    
    NULL,  // 0x1A = Reserved                    
    NULL,  // 0x1B = Reserved                    
    NULL,  // 0x1C = Reserved                    
    NULL,  // 0x1D = Reserved                    
    NULL,  // 0x1E = Reserved                    
    NULL,  // 0x1F = Reserved                    
};

struct S_COMMANDS_C1 COMMANDS_C1[32]=
{
    {CW0,"CW0","SetCurrentWindow0",     1},
    {CW1,"CW1","SetCurrentWindow1",     1},
    {CW2,"CW2","SetCurrentWindow2",     1},
    {CW3,"CW3","SetCurrentWindow3",     1},
    {CW4,"CW4","SetCurrentWindow4",     1},
    {CW5,"CW5","SetCurrentWindow5",     1},
    {CW6,"CW6","SetCurrentWindow6",     1},
    {CW7,"CW7","SetCurrentWindow7",     1},
    {CLW,"CLW","ClearWindows",          2},
    {DSW,"DSW","DisplayWindows",        2},
    {HDW,"HDW","HideWindows",           2},
    {TGW,"TGW","ToggleWindows",         2},
    {DLW,"DLW","DeleteWindows",         2},
    {DLY,"DLY","Delay",                 2},
    {DLC,"DLC","DelayCancel",           2},
    {RST,"RST","Reset",                 2},    
    {SPA,"SPA","SetPenAttributes",      3},
    {SPC,"SPC","SetPenColor",           4},
    {SPL,"SPL","SetPenLocation",        3},
    {RSV93,"RSV93","Reserved",          1},
    {RSV94,"RSV94","Reserved",          1},
    {RSV95,"RSV95","Reserved",          1},
    {RSV96,"RSV96","Reserved",          1},
    {SWA,"SWA","SetWindowAttributes",   5},
    {DF0,"DF0","DefineWindow0",         7},
    {DF1,"DF0","DefineWindow1",         7},
    {DF2,"DF0","DefineWindow2",         7},
    {DF3,"DF0","DefineWindow3",         7},
    {DF4,"DF0","DefineWindow4",         7},
    {DF5,"DF0","DefineWindow5",         7},
    {DF6,"DF0","DefineWindow6",         7},
    {DF7,"DF0","DefineWindow7",         7}
};


cc708_service_decoder decoders[2]; // For primary and secondary languages

void clear_packet(void)
{
        current_packet_length=0;
        memset (current_packet,0,MAX_708_PACKET_LENGTH);
}

void cc708_service_reset(cc708_service_decoder *decoder)
{
    memset (decoder,0,sizeof (cc708_service_decoder));
    // There's lots of other stuff that we need to do, such as canceling delays

}

void cc708_reset()
{
    printf (">>> Entry in cc708_reset()\n");
    // Clear states of decoders
    cc708_service_reset(&decoders[0]);
    cc708_service_reset(&decoders[1]);
    // Empty packet buffer
    clear_packet();
    last_seq=-1; 
    resets_708++;
}

int compWindowsPriorities (const void *a, const void *b)
{
    e708Window *w1=*(e708Window **)a;
    e708Window *w2=*(e708Window **)b;
    return w1->priority-w2->priority;
}

void clearTV (cc708_service_decoder *decoder)
{
    for (int i=0; i<I708_SCREENGRID_ROWS; i++)
    {
        memset (decoder->tv.chars[i], ' ', I708_SCREENGRID_COLUMNS);
        decoder->tv.chars[i][I708_SCREENGRID_COLUMNS]=0;
    }
};

void updateScreen (cc708_service_decoder *decoder)
{
    // THIS FUNCTION WILL DO THE MAGIC OF ACTUALLY EXPORTING THE DECODER STATUS
    // TO SEVERAL FILES
    e708Window *wnd[I708_MAX_WINDOWS]; // We'll store here the visible windows
    int visible=0;
    for (int i=0;i<I708_MAX_WINDOWS;i++)
    {
        if (decoder->windows[i].is_defined && decoder->windows[i].visible)
            wnd[visible++]=&decoder->windows[i];
    }
    qsort (wnd,visible,sizeof (int),compWindowsPriorities);
    printf ("Visible windows in priority order: ");
    for (int i=0;i<visible;i++)
    {
        printf ("%d (%d) | ",wnd[i]->number, wnd[i]->priority);
    }
    printf ("\n");
    clearTV(decoder);
    for (int i=0;i<visible;i++)
    {
        int top,left;
        // For each window we calculate the top,left position depending on the
        // anchor
        switch (wnd[i]->anchor_point)
        {
            case anchorpoint_top_left:
                top=wnd[i]->anchor_vertical;
                left=wnd[i]->anchor_horizontal;
                break;
            case anchorpoint_top_center:
                top=wnd[i]->anchor_vertical;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
                break;
            case anchorpoint_top_right:
                top=wnd[i]->anchor_vertical;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
                break;
            case anchorpoint_middle_left:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
                left=wnd[i]->anchor_horizontal;
                break;
            case anchorpoint_middle_center:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
                break;
            case anchorpoint_middle_right:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
                break;
            case anchorpoint_bottom_left:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count;
                left=wnd[i]->anchor_horizontal;
                break;
            case anchorpoint_bottom_center:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
                break;
            case anchorpoint_bottom_right:
                top=wnd[i]->anchor_vertical - wnd[i]->row_count;
                left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
                break;
            default: // Shouldn't happen, but skip the window just in case
                continue;
        }
        printf ("For window %d: Anchor point -> %d, size %d:%d, real position %d:%d\n",
            wnd[i]->number, wnd[i]->anchor_point, wnd[i]->row_count,wnd[i]->col_count,
            top,left);
        if (top<0)
            top=0;
        if (left<0)
            left=0;
        int copyrows=top + wnd[i]->row_count >= I708_SCREENGRID_ROWS ?
            I708_SCREENGRID_ROWS - top : wnd[i]->row_count;
        int copycols=left + wnd[i]->col_count >= I708_SCREENGRID_COLUMNS ?
            I708_SCREENGRID_COLUMNS - left : wnd[i]->col_count;
        printf ("%d*%d will be copied to the TV.\n", copyrows, copycols);
        for (int j=0;j<copyrows;j++)
        {
            memcpy (decoder->tv.chars[top+j],wnd[i]->rows[j],copycols);
        }
    }
}

void handle_708_extended_char (cc708_service_decoder *decoder, unsigned char code)
{
    printf ("In handle_708_extended_char: [%c]\n",code);
    unsigned char c=0x20; // Default to space
    if ((code>=0x20 && code<=0x3F) || 
        (code>=0x80 && code<=0x9F))
        c=get_internal_from_G2 (code);
    else if (code==0xa0)
        c=get_internal_from_G3 (code);
    process_character (decoder, c);
}

int handle_708_C0 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
    const char *name=COMMANDS_C0[data[0]];
    if (name==NULL)
        name="Reserved";
    printf ("C0: [%02X]  (%d)   [%s]\n",data[0],data_length, name);
    int len=-1;
    // These commands have a known length even if they are reserved. 
    if (/* data[0]>=0x00 && */ data[0]<=0xF) // Comment to silence warning
        len=1;
    else if (data[0]>=0x10 && data[0]<=0x17)
    {
        len=2;
    }
    else if (data[0]>=0x18 && data[0]<=0x1F)
        len=3;
    if (len==-1)
    {
        printf ("In handle_708_C0. Len == -1, this is not possible!");
        return -1;
    }
    if (len>data_length)
    {
        printf ("handle_708_C0, command is %d bytes long but we only have %d\n",len, data_length);
        return -1;
    }    
    // TODO: Do something useful eventually
    return len;
}

void process_character (cc708_service_decoder *decoder, unsigned char internal_char)
{    
    printf (">>> Process_character: %c [%02X]  - Window: %d, Pen: %d:%d\n", internal_char, internal_char,
        decoder->current_window,
        decoder->current_window!=-1 ? decoder->windows[decoder->current_window].pen_row:-1,
        decoder->current_window!=-1 ? decoder->windows[decoder->current_window].pen_column:-1
        );
    return; // TODO: Remove this
    decoder->windows[decoder->current_window].
        rows[decoder->windows[decoder->current_window].pen_row]
        [decoder->windows[decoder->current_window].pen_column]=internal_char;
    /* Not positive this interpretation is correct. Word wrapping is optional, so
       let's assume we don't need to autoscroll */
    switch (decoder->windows[decoder->current_window].attribs.print_dir)
    {
        case pd_left_to_right:
            if (decoder->windows[decoder->current_window].pen_column+1 < decoder->windows[decoder->current_window].col_count)
                decoder->windows[decoder->current_window].pen_column++;
            break;
        case pd_right_to_left:
            if (decoder->windows->pen_column>0)
                    decoder->windows[decoder->current_window].pen_column--;
            break;
        case pd_top_to_bottom:
            if (decoder->windows[decoder->current_window].pen_row+1 < decoder->windows[decoder->current_window].row_count)
                decoder->windows[decoder->current_window].pen_row++;
            break;
        case pd_bottom_to_top:
            if (decoder->windows[decoder->current_window].pen_row>0)
                    decoder->windows[decoder->current_window].pen_row--;
            break;
    }

}

int handle_708_G0 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
    printf ("G0: [%02X]  (%c)\n",data[0],data[0]);
    unsigned char c=get_internal_from_G0 (data[0]);
    process_character (decoder, c);
    return 1;
}

int handle_708_G1 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
    printf ("G1: [%02X]  (%c)\n",data[0],data[0]);
    unsigned char c=get_internal_from_G1 (data[0]);
    process_character (decoder, c);
    return 1;
}

/*-------------------------------------------------------
                    WINDOW COMMANDS
  ------------------------------------------------------- */                  
void handle_708_CWx_SetCurrentWindow (cc708_service_decoder *decoder, int new_window)
{
    printf ("    Entry in handle_708_CWx_SetCurrentWindow, new window: [%d]\n",new_window);
    if (decoder->windows[new_window].is_defined)
        decoder->current_window=new_window;
}

void clearWindow (cc708_service_decoder *decoder, int window)
{
    // TODO: Removes any existing text from the specified window. When a window
    // is cleared the entire window is filled with the window fill color.
}

void handle_708_CLW_ClearWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
    printf ("    Entry in handle_708_CLW_ClearWindows, windows: ");
    if (windows_bitmap==0)
        printf ("None\n");
    else
    {
        for (int i=0; i<8; i++)
        {
            if (windows_bitmap & 1)
            {
                printf ("[Window %d] ",i );
                clearWindow (decoder, i);                
            }
            windows_bitmap>>=1;
        }
    }
    printf ("\n");    
}

void handle_708_DSW_DisplayWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
    printf ("    Entry in handle_708_DSW_DisplayWindows, windows: ");    
    if (windows_bitmap==0)
        printf ("None\n");
    else
    {
        int changes=0;
        for (int i=0; i<8; i++)
        {
            if (windows_bitmap & 1)
            {
                printf ("[Window %d] ",i );
                if (!decoder->windows[i].visible)
                {
                    changes=1;
                    decoder->windows[i].visible=1;
                }                
            }
            windows_bitmap>>=1;
        }
        printf ("\n");    
        if (changes)
            updateScreen (decoder);
    }    
}

void handle_708_HDW_HideWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
    printf ("    Entry in handle_708_HDW_HideWindows, windows: ");
    if (windows_bitmap==0)
        printf ("None\n");
    else
    {
        int changes=0;
        for (int i=0; i<8; i++)
        {
            if (windows_bitmap & 1)
            {
                printf ("[Window %d] ",i );
                if (decoder->windows[i].visible)
                {
                    changes=1;
                    decoder->windows[i].visible=0;
                }
                // TODO: Actually Hide Window
            }
            windows_bitmap>>=1;
        }
        if (changes)
            updateScreen (decoder);
    }
    printf ("\n");    
}

void handle_708_TGW_ToggleWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
    printf ("    Entry in handle_708_TGW_ToggleWindows, windows: ");
    if (windows_bitmap==0)
        printf ("None\n");
    else
    {
        for (int i=0; i<8; i++)
        {
            if (windows_bitmap & 1)
            {
                printf ("[Window %d] ",i );
                decoder->windows[i].visible=!decoder->windows[i].visible;                
            }
            windows_bitmap>>=1;
        }
        updateScreen(decoder);
    }
    printf ("\n");    
}

void clearWindowText (e708Window *window)
{
        for (int i=0;i<I708_MAX_ROWS;i++)
        {
            memset (window->rows[i],' ',I708_MAX_COLUMNS);
            window->rows[i][I708_MAX_COLUMNS]=0;
        }
        memset (window->rows[I708_MAX_ROWS],0,I708_MAX_COLUMNS+1);

}

void handle_708_DFx_DefineWindow (cc708_service_decoder *decoder, int window, unsigned char *data)
{
    printf ("    Entry in handle_708_DFx_DefineWindow, window [%d], attributes: \n", window);
    if (decoder->windows[window].is_defined &&
        memcmp (decoder->windows[window].commands, data+1, 6)==0)
    {
        // When a decoder receives a DefineWindow command for an existing window, the
        // command is to be ignored if the command parameters are unchanged from the
        // previous window definition.
        printf ("       Repeated window definition, ignored.\n");
        return;
    }
    decoder->windows[window].number=window;
    int priority = (data[1]  ) & 0x7;
    int col_lock = (data[1]>>3) & 0x1;
    int row_lock = (data[1]>>4) & 0x1;
    int visible  = (data[1]>>5) & 0x1;
    int anchor_vertical = data[2] & 0x7f;
    int relative_pos = (data[2]>>7);
    int anchor_horizontal = data[3];
    int row_count = data[4] & 0xf;
    int anchor_point = data[4]>>4;
    int col_count = data[5] & 0x3f;    
    int pen_style = data[6] & 0x7;
    int win_style = (data[6]>>3) & 0x7;
    printf ("                   Priority: [%d]        Column lock: [%3s]      Row lock: [%3s]\n",
        priority, col_lock?"Yes": "No", row_lock?"Yes": "No");
    printf ("                    Visible: [%3s]  Anchor vertical: [%2d]   Relative pos: [%s]\n",
        visible?"Yes": "No", anchor_vertical, relative_pos?"Yes": "No");
    printf ("          Anchor horizontal: [%3d]        Row count: [%2d]+1  Anchor point: [%d]\n",
        anchor_horizontal, row_count, anchor_point);
    printf ("               Column count: [%2d]1      Pen style: [%d]      Win style: [%d]\n",
        col_count, pen_style, win_style);
    col_count++; // These increments seems to be needed but no documentation
    row_count++; // backs it up
    decoder->windows[window].priority=priority;
    decoder->windows[window].col_lock=col_lock;
    decoder->windows[window].row_lock=row_lock;
    decoder->windows[window].visible=visible;
    decoder->windows[window].anchor_vertical=anchor_vertical;
    decoder->windows[window].relative_pos=relative_pos;
    decoder->windows[window].anchor_horizontal=anchor_horizontal;
    decoder->windows[window].row_count=row_count;
    decoder->windows[window].anchor_point=anchor_point;
    decoder->windows[window].col_count=col_count;
    decoder->windows[window].pen_style=pen_style;
    decoder->windows[window].win_style=win_style;
    if (!decoder->windows[window].is_defined)
    {
        // If the window is being created, all character positions in the window
        // are set to the fill color...
        // TODO: COLORS
        // ...and the pen location is set to (0,0)        
        decoder->windows[window].pen_column=0;
        decoder->windows[window].pen_row=0;
        if (!decoder->windows[window].memory_reserved)
        {
            for (int i=0;i<=I708_MAX_ROWS;i++)
            {
                decoder->windows[window].rows[i]=(unsigned char *) malloc (I708_MAX_COLUMNS+1);
                if (decoder->windows[window].rows[i]==NULL) // Great
                {
                    decoder->windows[window].is_defined=0; 
                    decoder->current_window=-1;
                    for (int j=0;j<i;j++)                
                        free (decoder->windows[window].rows[j]);
                    return; // TODO: Warn somehow
                }
            }        
            decoder->windows[window].memory_reserved=1;
        }
        decoder->windows[window].is_defined=1;
        clearWindowText (&decoder->windows[window]);
    }
    else
    {
        // Specs unclear here: Do we need to delete the text in the existing window?
        // We do this because one of the sample files demands it.
       //  clearWindowText (&decoder->windows[window]);
    }    
    // ...also makes the defined windows the current window (setCurrentWindow)
    handle_708_CWx_SetCurrentWindow (decoder, window);
    memcpy (decoder->windows[window].commands, data+1, 6);
}

void handle_708_SWA_SetWindowAttributes (cc708_service_decoder *decoder, unsigned char *data)
{
    printf ("    Entry in handle_708_SWA_SetWindowAttributes, attributes: \n");
    int fill_color    = (data[1]   ) & 0x3f;
    int fill_opacity  = (data[1]>>6) & 0x03;
    int border_color  = (data[2]   ) & 0x3f;
    int border_type01 = (data[2]>>6) & 0x03;
    int justify       = (data[3]   ) & 0x03;
    int scroll_dir    = (data[3]>>2) & 0x03;
    int print_dir     = (data[3]>>4) & 0x03;
    int word_wrap     = (data[3]>>6) & 0x01;
    int border_type   = (data[3]>>5) | border_type01;
    int display_eff   = (data[4]   ) & 0x03;
    int effect_dir    = (data[4]>>2) & 0x03;
    int effect_speed  = (data[4]>>4) & 0x0f;
    printf ("       Fill color: [%d]     Fill opacity: [%d]    Border color: [%d]  Border type: [%d]\n",
        fill_color, fill_opacity, border_color, border_type01);
    printf ("          Justify: [%d]       Scroll dir: [%d]       Print dir: [%d]    Word wrap: [%d]\n",
        justify, scroll_dir, print_dir, word_wrap);
    printf ("      Border type: [%d]      Display eff: [%d]      Effect dir: [%d] Effect speed: [%d]\n",
        border_type, display_eff, effect_dir, effect_speed);
    if (decoder->current_window==-1)
    {
        // Can't do anything yet - we need a window to be defined first.
        return;
    }
    decoder->windows[decoder->current_window].attribs.fill_color=fill_color;
    decoder->windows[decoder->current_window].attribs.fill_opacity=fill_opacity;
    decoder->windows[decoder->current_window].attribs.border_color=border_color;
    decoder->windows[decoder->current_window].attribs.border_type01=border_type01;
    decoder->windows[decoder->current_window].attribs.justify=justify;
    decoder->windows[decoder->current_window].attribs.scroll_dir=scroll_dir;
    decoder->windows[decoder->current_window].attribs.print_dir=print_dir;
    decoder->windows[decoder->current_window].attribs.word_wrap=word_wrap;
    decoder->windows[decoder->current_window].attribs.border_type=border_type;
    decoder->windows[decoder->current_window].attribs.display_eff=display_eff;
    decoder->windows[decoder->current_window].attribs.effect_dir=effect_dir;
    decoder->windows[decoder->current_window].attribs.effect_speed=effect_speed;

}

void deleteWindow (cc708_service_decoder *decoder, int window)
{
    if (window==decoder->current_window)
    {
        // If the current window is deleted, then the decoder's current window ID
        // is unknown and must be reinitialized with either the SetCurrentWindow
        // or DefineWindow command.
        decoder->current_window=-1;
    }
    // TODO: Do the actual deletion (remove from display if needed, etc), mark as
    // not defined, etc
    if (decoder->windows[window].is_defined)
    {
        clearWindowText(&decoder->windows[window]);
    }
    decoder->windows[window].is_defined=0;
}

void handle_708_DLW_DeleteWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
    printf ("    Entry in handle_708_DLW_DeleteWindows, windows: ");
    if (windows_bitmap==0)
        printf ("None\n");
    else
    {
        for (int i=0; i<8; i++)
        {
            if (windows_bitmap & 1)
            {
                printf ("[Window %d] ",i );                
                deleteWindow (decoder, i);
            }
            windows_bitmap>>=1;
        }
    }
    printf ("\n");    
}

/*-------------------------------------------------------
                    WINDOW COMMANDS
  ------------------------------------------------------- */                  
void handle_708_SPA_SetPenAttributes (cc708_service_decoder *decoder, unsigned char *data)
{
    printf ("    Entry in handle_708_SPA_SetPenAttributes, attributes: \n");
    int pen_size  = (data[1]   ) & 0x3;
    int offset    = (data[1]>>2) & 0x3;
    int text_tag  = (data[1]>>4) & 0xf;
    int font_tag  = (data[2]   ) & 0x7;
    int edge_type = (data[2]>>3) & 0x7;
    int underline = (data[2]>>4) & 0x1;
    int italic    = (data[2]>>5) & 0x1;
    printf ("       Pen size: [%d]     Offset: [%d]  Text tag: [%d]   Font tag: [%d]\n",
        pen_size, offset, text_tag, font_tag);
    printf ("      Edge type: [%d]  Underline: [%d]    Italic: [%d]\n",
        edge_type, underline, italic);    
    if (decoder->current_window==-1)
    {
        // Can't do anything yet - we need a window to be defined first.
        return;
    }
    decoder->windows[decoder->current_window].pen.pen_size=pen_size;
    decoder->windows[decoder->current_window].pen.offset=offset;
    decoder->windows[decoder->current_window].pen.text_tag=text_tag;
    decoder->windows[decoder->current_window].pen.font_tag=font_tag;
    decoder->windows[decoder->current_window].pen.edge_type=edge_type;
    decoder->windows[decoder->current_window].pen.underline=underline;
    decoder->windows[decoder->current_window].pen.italic=italic;
}

void handle_708_SPC_SetPenColor (cc708_service_decoder *decoder, unsigned char *data)
{
    printf ("    Entry in handle_708_SPC_SetPenColor, attributes: \n");
    int fg_color   = (data[1]   ) & 0x3f;
    int fg_opacity = (data[1]>>6) & 0x03;
    int bg_color   = (data[2]   ) & 0x3f;
    int bg_opacity = (data[2]>>6) & 0x03;
    int edge_color = (data[3]>>6) & 0x3f;
    printf ("      Foreground color: [%d]     Foreground opacity: [%d]\n",
        fg_color, fg_opacity);
    printf ("      Background color: [%d]     Background opacity: [%d]\n",
        bg_color, bg_opacity);
    printf ("            Edge color: [%d]\n",
        edge_color);
    if (decoder->current_window==-1)
    {
        // Can't do anything yet - we need a window to be defined first.
        return;
    }

    decoder->windows[decoder->current_window].pen_color.fg_color=fg_color;
    decoder->windows[decoder->current_window].pen_color.fg_opacity=fg_opacity;
    decoder->windows[decoder->current_window].pen_color.bg_color=bg_color;
    decoder->windows[decoder->current_window].pen_color.bg_opacity=bg_opacity;
    decoder->windows[decoder->current_window].pen_color.edge_color=edge_color;
}


void handle_708_SPL_SetPenLocation (cc708_service_decoder *decoder, unsigned char *data)
{
    printf ("    Entry in handle_708_SPL_SetPenLocation, attributes: \n");
    int row = data[1] & 0x0f;
    int col = data[2] & 0x3f;
    printf ("      row: [%d]     Column: [%d]\n",
        row, col);
    if (decoder->current_window==-1)
    {
        // Can't do anything yet - we need a window to be defined first.
        return;
    }
    decoder->windows[decoder->current_window].pen_row=row;
    decoder->windows[decoder->current_window].pen_column=col;
}


/*-------------------------------------------------------
                 SYNCHRONIZATION COMMANDS
  ------------------------------------------------------- */                  
void handle_708_DLY_Delay (cc708_service_decoder *decoder, int tenths_of_sec)
{
    printf ("    Entry in handle_708_DLY_Delay, delay for [%d] tenths of second.", tenths_of_sec);
    // TODO: Probably ask for the current FTS and wait for this time before resuming -
    // not sure it's worth it though
}

void handle_708_DLC_DelayCancel (cc708_service_decoder *decoder)
{
    printf ("    Entry in handle_708_DLC_DelayCancel.");
    // TODO: See above
}


int handle_708_C1 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
    S_COMMANDS_C1 com=COMMANDS_C1[data[0]-0x80];
    printf ("%s | C1: [%02X]  [%s] [%s] (%d)\n",
        print_mstime(get_fts()),
        data[0],com.name,com.description, com.length);    
    if (com.length>data_length)
    {
        printf ("C1: Warning: Not enough bytes for command.\n");
        return -1;
    }
    switch (com.code)
    {
        case CW0: /* SetCurrentWindow */
        case CW1:
        case CW2:
        case CW3:
        case CW4:
        case CW5:
        case CW6:
        case CW7:
            handle_708_CWx_SetCurrentWindow (decoder, com.code-CW0); /* Window 0 to 7 */
            break;
        case CLW:
            handle_708_CLW_ClearWindows (decoder, data[1]); 
            break;
        case DSW:
            handle_708_DSW_DisplayWindows (decoder, data[1]);
            break;
        case HDW:
            handle_708_HDW_HideWindows (decoder, data[1]);
            break;
        case TGW:
            handle_708_TGW_ToggleWindows (decoder, data[1]);
            break;
        case DLW:
            handle_708_DLW_DeleteWindows (decoder, data[1]);
            break;
        case DLY:
            handle_708_DLY_Delay (decoder, data[1]);
            break;
        case DLC:
            handle_708_DLC_DelayCancel (decoder);
            break;
        case RST:
            cc708_service_reset(decoder);
            break;
        case SPA:
            handle_708_SPA_SetPenAttributes (decoder, data);
            break;
        case SPC:
            handle_708_SPC_SetPenColor (decoder, data);
            break;
        case SPL:
            handle_708_SPL_SetPenLocation (decoder, data);
            break;            
        case RSV93:
        case RSV94:
        case RSV95:
        case RSV96:
            printf ("Warning, found Reserved codes, ignored.\n");
            break;
        case SWA:
            handle_708_SWA_SetWindowAttributes (decoder, data);
            break;            
        case DF0:
        case DF1:
        case DF2:
        case DF3:
        case DF4:
        case DF5:
        case DF6:
        case DF7:
            handle_708_DFx_DefineWindow (decoder, com.code-DF0, data); /* Window 0 to 7 */
            break;            
        default:
            printf ("BUG: Unhandled code in handle_708_C1.\n");
            break;            
    }
    
    return com.length;
}


void process_service_block (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
    int i=0;    
    while (i<data_length)
    {
        if (data[i]==EXT1) // Use extended set
        {            
            handle_708_extended_char (decoder, data[i+1]);
            i+=2;
            continue;
        } 
        int used=-1;
        // Group C0
        if (/* data[i]>=0x00 && */ data[i]<=0x1F) // Comment to silence warning
        {
            used=handle_708_C0 (decoder,data+i,data_length-i);
        }
        // Group G0
        else if (data[i]>=0x20 && data[i]<=0x7F)
        {
            used=handle_708_G0 (decoder,data+i,data_length-i);
        }
        // Group C1
        else if (data[i]>=0x80 && data[i]<=0x9F)
        {
            used=handle_708_C1 (decoder,data+i,data_length-i);
        }
        // Group C2
        else
            used=handle_708_G1 (decoder,data+i,data_length-i);
        if (used==-1)
        {
            printf ("There was a problem handling the data. Reseting service decoder\n");
            // TODO: Not sure if a local reset is going to be helpful here.
            cc708_service_reset (decoder);
            return;   
        }
        i+=used; 
    }
}


void process_current_packet (void)
{
#ifdef DEBUG_708_PACKETS
    printf ("Processing EIA-708 packet, length=%d\n",current_packet_length);
#endif
    if (current_packet_length==0)
        return;
    int seq=(current_packet[0] & 0xC0) >> 6; // Two most significants bits
    int len=current_packet[0] & 0x3F; // 6 least significants bits
    if (len==0) // This is well defined in EIA-708; no magic.
        len=128; 
    else
        len=len*2;
    // Note that len here is the length including the header
#ifdef DEBUG_708_PACKETS            
    printf ("Sequence: %d, packet length: %d\n",seq,len);
#endif
    if (current_packet_length!=len) // Is this possible?
    {
        printf ("Packet length mismatch (%s%d), first two data bytes %02X %02X, current picture:%s\n", 
        current_packet_length-len>0?"+":"", current_packet_length-len, 
        current_packet[0], current_packet[1], pict_types[current_picture_coding_type]);
        cc708_reset();
        return;
    }
    if (last_seq!=-1 && (last_seq+1)%4!=seq)
    {
        printf ("Unexpected sequence number, it was [%d] but should have been [%d]\n",
            seq,(last_seq+1)%4);
        cc708_reset();
        return;
    }
    last_seq=seq;
   
    unsigned char *pos=current_packet+1;    

    while (pos<current_packet+len)
    {
        int service_number=(pos[0] & 0xE0)>>5; // 3 more significant bits
        int block_length = (pos[0] & 0x1F); // 5 less significant bits
#ifdef DEBUG_708_PACKETS        
       printf ("Standard header: Service number: [%d] Block length: [%d]\n",service_number,
            block_length); 
#endif
        if (service_number==7) // There is an extended header
        {
            pos++; 
            service_number=(pos[0] & 0x3F); // 6 more significant bits
            // printf ("Extended header: Service number: [%d]\n",service_number);
            if (service_number<7) 
            {
                printf ("Illegal service number in extended header: [%d]\n",service_number);
            }
        }            
        /*
        if (service_number==0 && block_length==0) // Null header already?
        {
            if (pos!=(current_packet+len-1)) // i.e. last byte in packet
            {
                // Not sure if this is correct
                printf ("Null header before it was expected.\n");            
            // break;
            }
        } */
        pos++; // Move to service data
        if (service_number==1) // Primary language        
            process_service_block (&decoders[0], pos, block_length);
        if (service_number==2) // Secondary language
            process_service_block (&decoders[1], pos, block_length);
        
        pos+=block_length; // Skip data    
    }

    clear_packet();

    if (pos!=current_packet+len) // For some reason we didn't parse the whole packet
    {
        printf ("There was a problem with this packet, reseting\n");
        cc708_reset();
    }

    if (len<128 && *pos) // Null header is mandatory if there is room
    {
        printf ("Warning: Null header expected but not found.\n");
    }    
}

void do_708 (const unsigned char *data, int datalength)
{
    /* Note: The data has this format: 
        1 byte for cc_valid        
        1 byte for cc_type
        2 bytes for the actual data */
    if (!debug_708)
        return;
        
    for (int i=0;i<datalength;i+=4)
    {
        unsigned char cc_valid=data[i];
        unsigned char cc_type=data[i+1];    

        switch (cc_type)
        {
            case 2:
                // printf ("708: DTVCC Channel Packet Data\n");
                if (cc_valid==0) // This ends the previous packet
                    process_current_packet();
                else
                {
                    if (current_packet_length>253) 
                    {
                        printf ("Warning: Legal packet size exceeded, data not added.\n");
                    }
                    else
                    {
                        current_packet[current_packet_length++]=data[i+2];
                        current_packet[current_packet_length++]=data[i+3];
                    }
                }
                break;
            case 3:                
                // printf ("708: DTVCC Channel Packet Start\n");
                process_current_packet();
                if (cc_valid)
                {
                    if (current_packet_length>253) 
                    {
                        printf ("Warning: Legal packet size exceeded, data not added.\n");
                    }
                    else
                    {
                        current_packet[current_packet_length++]=data[i+2];
                        current_packet[current_packet_length++]=data[i+3];
                    }
                }
                break;
            default:                
                fatal (EXIT_BUG_BUG, "708: shouldn't be here - cc_type: %d\n", cc_type);
        }
    }        
}

void init_708(void)
{
    for (int i=0;i<2;i++)
    {
        for (int j=0;j<8;j++)
        {
            decoders[i].windows[j].is_defined=0;
            decoders[i].windows[j].visible=0;
            decoders[i].windows[j].memory_reserved=0;
            memset (decoders[i].windows[j].commands, 0, 
                sizeof (decoders[i].windows[j].commands));            
        }
        decoders[i].current_window=-1;
        decoders[i].inited=1;
    }    
}
