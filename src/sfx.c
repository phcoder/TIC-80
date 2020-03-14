// MIT License

// Copyright (c) 2017 Vadim Grigoruk @nesbox // grigoruk@gmail.com

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "sfx.h"
#include "history.h"

#define DEFAULT_CHANNEL 0

enum 
{
	SFX_VOLUME_PANEL,
	SFX_CHORD_PANEL,
	SFX_PITCH_PANEL,
};

static inline void drawShadedText(tic_mem* tic, const char* text, s32 x, s32 y, u8 color)
{
	tic->api.text(tic, text, x, y+1, tic_color_0, false);
	tic->api.text(tic, text, x, y, color, false);
}

static void drawSwitch(Sfx* sfx, s32 x, s32 y, const char* label, s32 value, void(*set)(Sfx*, s32))
{
	static const u8 LeftArrow[] = 
	{
		0b00010000,
		0b00110000,
		0b01110000,
		0b00110000,
		0b00010000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	static const u8 RightArrow[] = 
	{
		0b01000000,
		0b01100000,
		0b01110000,
		0b01100000,
		0b01000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	drawShadedText(sfx->tic, label, x, y, tic_color_12);

	{
		x += (s32)strlen(label)*TIC_FONT_WIDTH;

		tic_rect rect = {x, y, TIC_FONT_WIDTH, TIC_FONT_HEIGHT};

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			if(checkMouseClick(&rect, tic_mouse_left))
				set(sfx, -1);
		}

		drawBitIcon(rect.x, rect.y+1, LeftArrow, tic_color_0);
		drawBitIcon(rect.x, rect.y, LeftArrow, tic_color_15);
	}

	{
		char val[] = "99";
		sprintf(val, "%02i", value);
		drawShadedText(sfx->tic, val, x += TIC_FONT_WIDTH, y, tic_color_12);
	}

	{
		x += 2*TIC_FONT_WIDTH;

		tic_rect rect = {x, y, TIC_FONT_WIDTH, TIC_FONT_HEIGHT};

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			if(checkMouseClick(&rect, tic_mouse_left))
				set(sfx, +1);
		}

		drawBitIcon(rect.x, rect.y+1, RightArrow, tic_color_0);
		drawBitIcon(rect.x, rect.y, RightArrow, tic_color_15);
	}
}

static tic_sample* getEffect(Sfx* sfx)
{
	return sfx->src->samples.data + sfx->index;
}

static void setIndex(Sfx* sfx, s32 delta)
{
	sfx->index += delta;
}

static void setSpeed(Sfx* sfx, s32 delta)
{
	tic_sample* effect = getEffect(sfx);

	effect->speed += delta;

	history_add(sfx->history);
}

static void drawStereoSwitch(Sfx* sfx, s32 x, s32 y)
{
	tic_sample* effect = getEffect(sfx);

	{
		tic_rect rect = {x, y, TIC_FONT_WIDTH, TIC_FONT_HEIGHT};

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			showTooltip("left stereo");

			if(checkMouseClick(&rect, tic_mouse_left))
				effect->stereo_left = !effect->stereo_left;
		}

		drawShadedText(sfx->tic, "L", x, y, effect->stereo_left ? tic_color_13 : tic_color_12);
	}

	{
		tic_rect rect = {x += TIC_FONT_WIDTH, y, TIC_FONT_WIDTH, TIC_FONT_HEIGHT};

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			showTooltip("right stereo");

			if(checkMouseClick(&rect, tic_mouse_left))
				effect->stereo_right = !effect->stereo_right;
		}

		drawShadedText(sfx->tic, "R", x, y, effect->stereo_right ? tic_color_13 : tic_color_12);
	}
}

static void drawTopPanel(Sfx* sfx, s32 x, s32 y)
{
	const s32 Gap = 8*TIC_FONT_WIDTH;

	drawSwitch(sfx, x, y, "IDX", sfx->index, setIndex);

	tic_sample* effect = getEffect(sfx);

	drawSwitch(sfx, x += Gap, y, "SPD", effect->speed, setSpeed);

	drawStereoSwitch(sfx, x += Gap, y);
}

static void setLoopStart(Sfx* sfx, s32 delta, s32 canvasTab)
{
	tic_sample* effect = getEffect(sfx);
	tic_sound_loop* loop = effect->loops + canvasTab;

	loop->start += delta;

	history_add(sfx->history);
}

static void setLoopSize(Sfx* sfx, s32 delta, s32 canvasTab)
{
	tic_sample* effect = getEffect(sfx);
	tic_sound_loop* loop = effect->loops + canvasTab;

	loop->size += delta;

	history_add(sfx->history);
}

static void drawLoopPanel(Sfx* sfx, s32 x, s32 y)
{
	// drawShadedText(sfx->tic, "LOOP:", x, y, tic_color_13);

	// enum {Gap = 2};

	// tic_sample* effect = getEffect(sfx);
	// tic_sound_loop* loop = effect->loops + sfx->canvasTab;

	// drawSwitch(sfx, x, y += Gap + TIC_FONT_HEIGHT, "", loop->size, setLoopSize);
	// drawSwitch(sfx, x, y += Gap + TIC_FONT_HEIGHT, "", loop->start, setLoopStart);
}

static tic_waveform* getWaveformById(Sfx* sfx, s32 i)
{
	return &sfx->src->waveforms.items[i];
}

// static tic_waveform* getWaveform(Sfx* sfx)
// {
// 	return getWaveformById(sfx, sfx->waveform.index);
// }

static void drawWaveButtons(Sfx* sfx, s32 x, s32 y)
{
	static const u8 EmptyIcon[] = 
	{
		0b01110000,
		0b10001000,
		0b10001000,
		0b10001000,
		0b01110000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	static const u8 FullIcon[] = 
	{
		0b01110000,
		0b11111000,
		0b11111000,
		0b11111000,
		0b01110000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	enum {Scale = 4, Width = 10, Height = 5, Gap = 1, Count = WAVES_COUNT, Rows = Count, HGap = 2};

	for(s32 i = 0; i < Count; i++)
	{
		tic_rect rect = {x, y + (Count - i - 1)*(Height+Gap), Width, Height};

		bool over = false;

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);
			over = true;

			if(checkMouseClick(&rect, tic_mouse_left))
			{
				// sfx->waveform.index = i;
				// sfx->tab = SFX_WAVEFORM_TAB;
				return;
			}
		}

		bool active = false;
		if(sfx->play.active)
		{
			tic_sfx_pos pos = sfx->tic->api.sfx_pos(sfx->tic, DEFAULT_CHANNEL);

			if(pos.wave >= 0 && getEffect(sfx)->data[pos.wave].wave == i)
				active = true;
		}

		sfx->tic->api.rect(sfx->tic, rect.x, rect.y, rect.w, rect.h, 
			active ? tic_color_2 : over ? tic_color_13 : tic_color_15);

		{
			enum{Size = 5};
			tic_rect iconRect = {x+Width+HGap, y + (Count - i - 1)*(Height+Gap), Size, Size};

			bool over = false;
			if(checkMousePos(&iconRect))
			{
				setCursor(tic_cursor_hand);
				over = true;

				if(checkMouseClick(&iconRect, tic_mouse_left))
				{
					tic_sample* effect = getEffect(sfx);
					for(s32 c = 0; c < SFX_TICKS; c++)
						effect->data[c].wave = i;
				}
			}

			drawBitIcon(iconRect.x, iconRect.y, EmptyIcon, (over ? tic_color_15 : tic_color_13));
		}

		{
			tic_waveform* wave = getWaveformById(sfx, i);

			for(s32 i = 0; i < WAVE_VALUES/Scale; i++)
			{
				s32 value = tic_tool_peek4(wave->data, i*Scale)/Scale;
				sfx->tic->api.pixel(sfx->tic, rect.x + i+1, rect.y + Height - value - 2, tic_color_12);
			}
		}
	}

	// draw full icon
	{
		tic_sample* effect = getEffect(sfx);
		u8 start = effect->data[0].wave;
		bool full = true;
		for(s32 c = 1; c < SFX_TICKS; c++)
			if(effect->data[c].wave != start)
			{
				full = false;
				break;
			}

		if(full)
			drawBitIcon(x+Width+HGap, y + (Count - start - 1)*(Height+Gap), FullIcon, tic_color_12);
	}
}

// static void drawCanvasTabs(Sfx* sfx, s32 x, s32 y)
// {
// 	static const char* Labels[] = {"WAVE", "VOLUME", "CHORD", "PITCH"};

// 	enum {Height = TIC_FONT_HEIGHT+2};

// 	for(s32 i = 0, sy = y; i < COUNT_OF(Labels); sy += Height, i++)
// 	{
// 		s32 size = sfx->tic->api.text(sfx->tic, Labels[i], 0, -TIC_FONT_HEIGHT, tic_color_0, false);

// 		tic_rect rect = {x - size, sy, size, TIC_FONT_HEIGHT};

// 		if(checkMousePos(&rect))
// 		{
// 			setCursor(tic_cursor_hand);

// 			if(checkMouseClick(&rect, tic_mouse_left))
// 			{
// 				sfx->canvasTab = i;
// 			}
// 		}

// 		drawShadedText(sfx->tic, Labels[i], rect.x, rect.y, i == sfx->canvasTab ? tic_color_12 : tic_color_13);
// 	}

// 	tic_sample* effect = getEffect(sfx);

// 	switch(sfx->canvasTab)
// 	{
// 	case SFX_PITCH_PANEL:
// 		{
// 			static const char Label[] = "x16";
// 			enum{Width = (sizeof Label - 1) * TIC_FONT_WIDTH};
// 			tic_rect rect = {(x - Width)/2, y + Height * 6, Width, TIC_FONT_HEIGHT};

// 			if(checkMousePos(&rect))
// 			{
// 				setCursor(tic_cursor_hand);

// 				if(checkMouseClick(&rect, tic_mouse_left))
// 					effect->pitch16x++;
// 			}

// 			drawShadedText(sfx->tic, Label, rect.x, rect.y, effect->pitch16x ? tic_color_12 : tic_color_13);
// 		}
// 		break;
// 	case SFX_CHORD_PANEL:
// 		{
// 			static const char Label[] = "DOWN";
// 			enum{Width = (sizeof Label - 1) * TIC_FONT_WIDTH};
// 			tic_rect rect = {(x - Width)/2, y + Height * 6, Width, TIC_FONT_HEIGHT};

// 			if(checkMousePos(&rect))
// 			{
// 				setCursor(tic_cursor_hand);

// 				if(checkMouseClick(&rect, tic_mouse_left))
// 					effect->reverse++;
// 			}

// 			drawShadedText(sfx->tic, Label, rect.x, rect.y, effect->reverse ? tic_color_12 : tic_color_13);
// 		}   
// 		break;
// 	default: break;
// 	}
// }

static void drawPanelBorder(tic_mem* tic, s32 x, s32 y, s32 w, s32 h, tic_color color)
{
	tic->api.rect(tic, x, y, w, h, color);

	tic->api.rect(tic, x, y-1, w, 1, tic_color_15);
	tic->api.rect(tic, x-1, y, 1, h, tic_color_15);
	tic->api.rect(tic, x, y+h, w, 1, tic_color_13);
	tic->api.rect(tic, x+w, y, 1, h, tic_color_13);
}

static void drawCanvas(Sfx* sfx, s32 x, s32 y, s32 canvasTab)
{
	tic_mem* tic = sfx->tic;

	enum 
	{
		Cols = SFX_TICKS, Rows = 16, 
		Gap = 1, LedWidth = 3 + Gap, LedHeight = 1 + Gap,
		Width = LedWidth * Cols + Gap, 
		Height = LedHeight * Rows + Gap
	};

	drawPanelBorder(tic, x, y, Width, Height, tic_color_15);

	for(s32 i = 0; i < Height; i += LedHeight)
		tic->api.rect(tic, x, y + i, Width, Gap, tic_color_0);

	for(s32 i = 0; i < Width; i += LedWidth)
		tic->api.rect(tic, x + i, y, Gap, Height, tic_color_0);

	{
		tic_sfx_pos pos = tic->api.sfx_pos(tic, DEFAULT_CHANNEL);
		s32 tickIndex = *(pos.data + canvasTab);

		if(tickIndex >= 0)
			tic->api.rect(tic, x + tickIndex * LedWidth, y, LedWidth + 1, Height, tic_color_12);
	}

	tic_rect rect = {x, y, Width - Gap, Height - Gap};

	tic_sample* effect = getEffect(sfx);

	if(checkMousePos(&rect))
	{
		setCursor(tic_cursor_hand);

		s32 mx = getMouseX() - x;
		s32 my = getMouseY() - y;

		if(checkMouseDown(&rect, tic_mouse_left))
		{
			mx /= LedWidth;
			my /= LedHeight;

			switch(canvasTab)
			{
			case SFX_VOLUME_PANEL:    effect->data[mx].volume = my; break;
			case SFX_CHORD_PANEL:     effect->data[mx].chord = Rows - my - 1; break;
			case SFX_PITCH_PANEL:     effect->data[mx].pitch = Rows / 2 - my - 1; break;
			// case SFX_WAVE_TAB:      effect->data[mx].wave = Rows - my - 1; break;
			default: break;
			}

			history_add(sfx->history);
		}
	}

	for(s32 i = 0; i < Cols; i++)
	{
		switch(canvasTab)
		{
		case SFX_VOLUME_PANEL:
			for(s32 j = 1, start = Height - LedHeight, value = Rows - effect->data[i].volume; j <= value; j++, start -= LedHeight)
				tic->api.rect(tic, x + i * LedWidth + Gap, y + start, LedWidth-Gap, LedHeight-Gap, j == value ? tic_color_9 : tic_color_10);				
			break;

		case SFX_CHORD_PANEL:
			for(s32 j = 1, start = Height - LedHeight, value = effect->data[i].chord + 1; j <= value; j++, start -= LedHeight)
				tic->api.rect(tic, x + i * LedWidth + Gap, y + start, LedWidth-Gap, LedHeight-Gap, j == value ? tic_color_6 : tic_color_5);
			break;

		case SFX_PITCH_PANEL:
			for(s32 value = effect->data[i].pitch, j = MIN(0, value); j <= MAX(0, value); j++)
				tic->api.rect(tic, x + i * LedWidth + Gap, y + (Height / 2 - (j + 1) * LedHeight + Gap),
					LedWidth-Gap, LedHeight-Gap, j == value ? tic_color_3 : tic_color_4);
			break;

		// case SFX_WAVE_TAB:
		// 		drawLed(tic, x + i * CANVAS_SIZE, y + (CANVAS_HEIGHT - (effect->data[i].wave+1)*CANVAS_SIZE));
		// 	break;
		}
	}

	// {
	// 	tic_sound_loop* loop = effect->loops + canvasTab;
	// 	if(loop->start > 0 || loop->size > 0)
	// 	{
	// 		for(s32 i = 0; i < loop->size; i++)
	// 			tic->api.rect(tic, x + (loop->start+i) * CANVAS_SIZE+1, y + CANVAS_HEIGHT - 1, CANVAS_SIZE-1, 2, tic_color_4);
	// 	}
	// }
}

// static void drawPiano2(Sfx* sfx, s32 x, s32 y)
// {
// 	tic_sample* effect = getEffect(sfx);

// 	static const s32 ButtonIndixes[] = {0, 2, 4, 5, 7, 9, 11, 1, 3, -1, 6, 8, 10};

// 	tic_rect buttons[COUNT_OF(ButtonIndixes)];

// 	for(s32 i = 0; i < COUNT_OF(buttons); i++)
// 	{
// 		buttons[i] = i < PIANO_WHITE_BUTTONS 
// 			? (tic_rect){x + (PIANO_BUTTON_WIDTH+1)*i, y, PIANO_BUTTON_WIDTH + 1, PIANO_BUTTON_HEIGHT}
// 			: (tic_rect){x + (7 + 3) * (i - PIANO_WHITE_BUTTONS) + 6, y, 7, 8};
// 	}

// 	tic_rect rect = {x, y, PIANO_WIDTH, PIANO_HEIGHT};

// 	if(checkMousePos(&rect))
// 	{
// 		setCursor(tic_cursor_hand);

// 		static const char* Tooltips[] = {"C [z]", "C# [s]", "D [x]", "D# [d]", "E [c]", "F [v]", "F# [g]", "G [b]", "G# [h]", "A [n]", "A# [j]", "B [m]" };

// 		for(s32 i = COUNT_OF(buttons) - 1; i >= 0; i--)
// 		{
// 			tic_rect* rect = buttons + i;

// 			if(checkMousePos(rect))
// 				if(ButtonIndixes[i] >= 0)
// 				{
// 					showTooltip(Tooltips[ButtonIndixes[i]]);
// 					break;
// 				}
// 		}

// 		if(checkMouseDown(&rect, tic_mouse_left))
// 		{
// 			for(s32 i = COUNT_OF(buttons) - 1; i >= 0; i--)
// 			{
// 				tic_rect* rect = buttons + i;
// 				s32 index = ButtonIndixes[i];

// 				if(index >= 0)
// 				{
// 					if(checkMousePos(rect))
// 					{
// 						effect->note = index;
// 						sfx->play.active = true;
// 						break;
// 					}
// 				}
// 			}           
// 		}
// 	}

// 	for(s32 i = 0; i < COUNT_OF(buttons); i++)
// 	{
// 		tic_rect* rect = buttons + i;
// 		bool white = i < PIANO_WHITE_BUTTONS;
// 		s32 index = ButtonIndixes[i];

// 		if(index >= 0)
// 			sfx->tic->api.rect(sfx->tic, rect->x, rect->y, rect->w - (white ? 1 : 0), rect->h, 
// 				(sfx->play.active && effect->note == index ? tic_color_2 : white ? tic_color_12 : tic_color_0));
// 	}
// }

static void drawOctavePanel(Sfx* sfx, s32 x, s32 y)
{
	tic_sample* effect = getEffect(sfx);

	static const char Label[] = "OCT";
	drawShadedText(sfx->tic, Label, x, y, tic_color_12);

	x += sizeof(Label)*TIC_FONT_WIDTH;

	enum {Gap = 5};

	for(s32 i = 0; i < OCTAVES; i++)
	{
		tic_rect rect = {x + i * (TIC_FONT_WIDTH + Gap), y, TIC_FONT_WIDTH, TIC_FONT_HEIGHT};

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			if(checkMouseClick(&rect, tic_mouse_left))
			{
				effect->octave = i;
			}
		}

		drawShadedText(sfx->tic, (char[]){i + '1'}, rect.x, rect.y, i == effect->octave ? tic_color_12 : tic_color_13);
	}
}

static void playSound(Sfx* sfx)
{
	if(sfx->play.active)
	{
		tic_sample* effect = getEffect(sfx);

		if(sfx->play.note != effect->note)
		{
			sfx->play.note = effect->note;
			sfx->tic->api.sfx_stop(sfx->tic, DEFAULT_CHANNEL);
			sfx->tic->api.sfx(sfx->tic, sfx->index, effect->note, effect->octave, -1, DEFAULT_CHANNEL);
		}
	}
	else
	{
		sfx->play.note = -1;
		sfx->tic->api.sfx_stop(sfx->tic, DEFAULT_CHANNEL);
	}
}

static void undo(Sfx* sfx)
{
	history_undo(sfx->history);
}

static void redo(Sfx* sfx)
{
	history_redo(sfx->history);
}

static void copyToClipboard(Sfx* sfx)
{
	tic_sample* effect = getEffect(sfx);
	toClipboard(effect, sizeof(tic_sample), true);
}

static void copyWaveToClipboard(Sfx* sfx)
{
	// tic_waveform* wave = getWaveform(sfx);
	// toClipboard(wave, sizeof(tic_waveform), true);
}

static void resetSfx(Sfx* sfx)
{
	tic_sample* effect = getEffect(sfx);
	memset(effect, 0, sizeof(tic_sample));

	history_add(sfx->history);
}

static void resetWave(Sfx* sfx)
{
	// tic_waveform* wave = getWaveform(sfx);
	// memset(wave, 0, sizeof(tic_waveform));

	// history_add(sfx->history);
}

static void cutToClipboard(Sfx* sfx)
{
	copyToClipboard(sfx);
	resetSfx(sfx);
}

static void cutWaveToClipboard(Sfx* sfx)
{
	copyWaveToClipboard(sfx);
	resetWave(sfx);
}

static void copyFromClipboard(Sfx* sfx)
{
	tic_sample* effect = getEffect(sfx);

	if(fromClipboard(effect, sizeof(tic_sample), true, false))
		history_add(sfx->history);
}

static void copyWaveFromClipboard(Sfx* sfx)
{
	// tic_waveform* wave = getWaveform(sfx);

	// if(fromClipboard(wave, sizeof(tic_waveform), true, false))
	// 	history_add(sfx->history);
}

static void processKeyboard(Sfx* sfx)
{
	tic_mem* tic = sfx->tic;

	if(tic->ram.input.keyboard.data == 0) return;

	bool ctrl = tic->api.key(tic, tic_key_ctrl);

	s32 keyboardButton = -1;

	static const s32 Keycodes[] = 
	{
		tic_key_z,
		tic_key_s,
		tic_key_x,
		tic_key_d,
		tic_key_c,
		tic_key_v,
		tic_key_g,
		tic_key_b,
		tic_key_h,
		tic_key_n,
		tic_key_j,
		tic_key_m,
	};

	if(ctrl)
	{

	}
	else
	{
		for(int i = 0; i < COUNT_OF(Keycodes); i++)
			if(tic->api.key(tic, Keycodes[i]))
				keyboardButton = i;        
	}

	tic_sample* effect = getEffect(sfx);

	if(keyboardButton >= 0)
	{
		effect->note = keyboardButton;
		sfx->play.active = true;
	}

	if(tic->api.key(tic, tic_key_space))
		sfx->play.active = true;
}

static void processEnvelopesKeyboard(Sfx* sfx)
{
	tic_mem* tic = sfx->tic;
	bool ctrl = tic->api.key(tic, tic_key_ctrl);

	switch(getClipboardEvent())
	{
	case TIC_CLIPBOARD_CUT: cutToClipboard(sfx); break;
	case TIC_CLIPBOARD_COPY: copyToClipboard(sfx); break;
	case TIC_CLIPBOARD_PASTE: copyFromClipboard(sfx); break;
	default: break;
	}

	if(ctrl)
	{
		if(keyWasPressed(tic_key_z))        undo(sfx);
		else if(keyWasPressed(tic_key_y))   redo(sfx);
	}

	// if(keyWasPressed(tic_key_tab))          sfx->tab = SFX_WAVEFORM_TAB;
	else if(keyWasPressed(tic_key_left))    sfx->index--;
	else if(keyWasPressed(tic_key_right))   sfx->index++;
	else if(keyWasPressed(tic_key_delete))  resetSfx(sfx);
}

static void processWaveformKeyboard(Sfx* sfx)
{
	tic_mem* tic = sfx->tic;

	bool ctrl = tic->api.key(tic, tic_key_ctrl);

	switch(getClipboardEvent())
	{
	case TIC_CLIPBOARD_CUT: cutWaveToClipboard(sfx); break;
	case TIC_CLIPBOARD_COPY: copyWaveToClipboard(sfx); break;
	case TIC_CLIPBOARD_PASTE: copyWaveFromClipboard(sfx); break;
	default: break;
	}

	if(ctrl)
	{
		if(keyWasPressed(tic_key_z))        undo(sfx);
		else if(keyWasPressed(tic_key_y))   redo(sfx);
	}

	// if(keyWasPressed(tic_key_tab))          sfx->tab = SFX_ENVELOPES_TAB;
	// else if(keyWasPressed(tic_key_left))    sfx->waveform.index--;
	// else if(keyWasPressed(tic_key_right))   sfx->waveform.index++;
	else if(keyWasPressed(tic_key_delete))  resetWave(sfx);
}

static void drawModeTabs(Sfx* sfx)
{
	// static const u8 Icons[] =
	// {
	// 	0b00000000,
	// 	0b00110000,
	// 	0b01001001,
	// 	0b01001001,
	// 	0b01001001,
	// 	0b00000110,
	// 	0b00000000,
	// 	0b00000000,

	// 	0b00000000,
	// 	0b01000000,
	// 	0b01000100,
	// 	0b01010100,
	// 	0b01010101,
	// 	0b01010101,
	// 	0b00000000,
	// 	0b00000000,
	// };

	// enum { Width = 9, Height = 7, Rows = 8, Count = sizeof Icons / Rows };

	// for (s32 i = 0; i < Count; i++)
	// {
	// 	tic_rect rect = { TIC80_WIDTH - Width * (Count - i), 0, Width, Height };

	// 	bool over = false;

	// 	static const s32 Tabs[] = { SFX_WAVEFORM_TAB, SFX_ENVELOPES_TAB };

	// 	if (checkMousePos(&rect))
	// 	{
	// 		setCursor(tic_cursor_hand);
	// 		over = true;

	// 		static const char* Tooltips[] = { "WAVEFORMS [tab]", "ENVELOPES [tab]" };
	// 		showTooltip(Tooltips[i]);

	// 		if (checkMouseClick(&rect, tic_mouse_left))
	// 			sfx->tab = Tabs[i];
	// 	}

	// 	if (sfx->tab == Tabs[i])
	// 		sfx->tic->api.rect(sfx->tic, rect.x, rect.y, rect.w, rect.h, tic_color_14);

	// 	drawBitIcon(rect.x, rect.y, Icons + i*Rows, (sfx->tab == Tabs[i] ? tic_color_12 : over ? tic_color_14 : tic_color_13));
	// }
}

static void drawSfxToolbar(Sfx* sfx)
{
	sfx->tic->api.rect(sfx->tic, 0, 0, TIC80_WIDTH, TOOLBAR_SIZE, tic_color_12);

	enum{Width = 3 * TIC_FONT_WIDTH};
	s32 x = TIC80_WIDTH - Width - TIC_SPRITESIZE*3;
	s32 y = 1;

	tic_rect rect = {x, y, Width, TIC_FONT_HEIGHT};
	bool over = false;

	if(checkMousePos(&rect))
	{
		setCursor(tic_cursor_hand);
		over = true;

		showTooltip("PLAY SFX [space]");

		if(checkMouseDown(&rect, tic_mouse_left))
		{
			sfx->play.active = true;
		}
	}

	tic_sample* effect = getEffect(sfx);

	{
		static const char* Notes[] = SFX_NOTES;
		char buf[] = "C#4";
		sprintf(buf, "%s%i", Notes[effect->note], effect->octave+1);

		sfx->tic->api.fixed_text(sfx->tic, buf, x, y, (over ? tic_color_14 : tic_color_13), false);
	}

	drawModeTabs(sfx);
}

static void envelopesTick(Sfx* sfx)
{
	processKeyboard(sfx);
	processEnvelopesKeyboard(sfx);

	sfx->tic->api.clear(sfx->tic, tic_color_14);

	enum{ Gap = 3, Start = 40};
	// drawPiano2(sfx, Start, TIC80_HEIGHT - PIANO_HEIGHT - Gap);
	drawTopPanel(sfx, Start, TOOLBAR_SIZE + Gap);

	drawSfxToolbar(sfx);
	drawToolbar(sfx->tic, false);

	// drawCanvasTabs(sfx, Start-Gap, TOOLBAR_SIZE + Gap + TIC_FONT_HEIGHT+2);
	// if(sfx->canvasTab == SFX_WAVE_TAB)
	// 	drawWaveButtons(sfx, Start + CANVAS_WIDTH + Gap-1, TOOLBAR_SIZE + Gap + TIC_FONT_HEIGHT+2);

	drawLoopPanel(sfx, Gap, TOOLBAR_SIZE + Gap + TIC_FONT_HEIGHT+92);
	// drawCanvas(sfx, Start-1, TOOLBAR_SIZE + Gap + TIC_FONT_HEIGHT + 1);
	// drawOctavePanel(sfx, Start + Gap + PIANO_WIDTH + Gap-1, TIC80_HEIGHT - TIC_FONT_HEIGHT - (PIANO_HEIGHT - TIC_FONT_HEIGHT)/2 - Gap);
}

static void drawWaveformBar(Sfx* sfx, s32 x, s32 y)
{
	// enum 
	// {
	// 	Border = 2,
	// 	Scale = 2,
	// 	Width = WAVE_VALUES/Scale + Border, 
	// 	Height = CANVAS_HEIGHT/CANVAS_SIZE/Scale + Border,
	// 	Gap = 3,
	// 	Rows = 2,
	// 	Cols = WAVES_COUNT/Rows,
	// };

	// for(s32 i = 0; i < WAVES_COUNT; i++)
	// {
	// 	tic_rect rect = {x + (i%Cols)*(Width+Gap), y + (i/Cols)*(Height+Gap), Width, Height};

	// 	if(checkMousePos(&rect))
	// 	{
	// 		setCursor(tic_cursor_hand);

	// 		if(checkMouseClick(&rect, tic_mouse_left))
	// 			sfx->waveform.index = i;
	// 	}

	// 	bool active = false;
	// 	if(sfx->play.active)
	// 	{
	// 		tic_sfx_pos pos = sfx->tic->api.sfx_pos(sfx->tic, DEFAULT_CHANNEL);

	// 		if(pos.wave >= 0 && getEffect(sfx)->data[pos.wave].wave == i)
	// 			active = true;
	// 	}

	// 	sfx->tic->api.rect(sfx->tic, rect.x, rect.y, rect.w, rect.h, (active ? tic_color_2 : tic_color_12));

	// 	if(sfx->waveform.index == i)
	// 		sfx->tic->api.rect_border(sfx->tic, rect.x-2, rect.y-2, rect.w+4, rect.h+4, tic_color_12);

	// 	{
	// 		tic_waveform* wave = getWaveformById(sfx, i);

	// 		for(s32 i = 0; i < WAVE_VALUES/Scale; i++)
	// 		{
	// 			s32 value = tic_tool_peek4(wave->data, i*Scale)/Scale;
	// 			sfx->tic->api.pixel(sfx->tic, rect.x + i+1, rect.y + Height - value - 2, tic_color_0);
	// 		}
	// 	}
	// }
}

static void drawWaveformCanvas(Sfx* sfx, s32 x, s32 y)
{
	// enum {Rows = CANVAS_ROWS, Width = WAVE_VALUES * CANVAS_SIZE, Height = CANVAS_HEIGHT};

	// tic_rect rect = {x, y, Width, Height};

	// sfx->tic->api.rect(sfx->tic, rect.x, rect.y, rect.w, rect.h, tic_color_0);

	// for(s32 i = 0; i < Height; i += CANVAS_SIZE)
	// 	sfx->tic->api.line(sfx->tic, rect.x, rect.y + i, rect.x + Width, rect.y + i, tic_color_14);

	// for(s32 i = 0; i < Width; i += CANVAS_SIZE)
	// 	sfx->tic->api.line(sfx->tic, rect.x + i, rect.y, rect.x + i, rect.y + Width, tic_color_14);

	// if(checkMousePos(&rect))
	// {
	// 	setCursor(tic_cursor_hand);

	// 	if(checkMouseDown(&rect, tic_mouse_left))
	// 	{
	// 		s32 mx = getMouseX() - x;
	// 		s32 my = getMouseY() - y;

	// 		mx -= mx % CANVAS_SIZE;
	// 		my -= my % CANVAS_SIZE;

	// 		mx /= CANVAS_SIZE;
	// 		my /= CANVAS_SIZE;

	// 		tic_waveform* wave = getWaveform(sfx);

	// 		tic_tool_poke4(wave->data, mx, Rows - my - 1);

	// 		history_add(sfx->history);
	// 	}
	// }

	// tic_waveform* wave = getWaveform(sfx);

	// for(s32 i = 0; i < WAVE_VALUES; i++)
	// {
	// 	// s32 value = tic_tool_peek4(wave->data, i);
	// 	// drawLed(sfx->tic, x + i * CANVAS_SIZE, y + (Height - (value+1)*CANVAS_SIZE));
	// }
}

static void waveformTick(Sfx* sfx)
{
	processKeyboard(sfx);
	processWaveformKeyboard(sfx);

	sfx->tic->api.clear(sfx->tic, tic_color_14);

	drawSfxToolbar(sfx);
	drawToolbar(sfx->tic, false);

	drawWaveformCanvas(sfx, 23, 11);
	drawWaveformBar(sfx, 36, 110);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

static void drawWaves(Sfx* sfx, s32 x, s32 y)
{
	tic_mem* tic = sfx->tic;

	enum{Width = 10, Height = 6, MarginRight = 6, MarginBottom = 4, Cols = 4, Rows = 4, Scale = 4};

	for(s32 i = 0; i < WAVES_COUNT; i++)
	{
		s32 xi = i % Cols;
		s32 yi = i / Cols;

		tic_rect rect = {x + xi * (Width + MarginRight), y + yi * (Height + MarginBottom), Width, Height};
		tic_sample* effect = getEffect(sfx);

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			if(checkMouseClick(&rect, tic_mouse_left))
			{
				for(s32 c = 0; c < SFX_TICKS; c++)
					effect->data[c].wave = i;

				history_add(sfx->history);
			}
		}

		bool sel = i == effect->data->wave;

		tic_sfx_pos pos = tic->api.sfx_pos(tic, DEFAULT_CHANNEL);
		bool active = *pos.data < 0 ? false : i == effect->data[*pos.data].wave;

		drawPanelBorder(tic, rect.x, rect.y, rect.w, rect.h, active ? tic_color_3 : sel ? tic_color_5 : tic_color_0);

		// draw tiny wave previews
		{
			tic_waveform* wave = getWaveformById(sfx, i);

			for(s32 i = 0; i < WAVE_VALUES/Scale; i++)
			{
				s32 value = tic_tool_peek4(wave->data, i*Scale)/Scale;
				tic->api.pixel(tic, rect.x + i+1, rect.y + Height - value - 2, active ? tic_color_2 : sel ? tic_color_7 : tic_color_12);
			}

			// draw flare
			if(sel || active)
			{
				tic->api.rect(tic, rect.x + rect.w - 2, rect.y, 2, 1, tic_color_12);
				tic->api.pixel(tic, rect.x + rect.w - 1, rect.y + 1, tic_color_12);
			}
		}

	}
}

static void drawWavePanel(Sfx* sfx, s32 x, s32 y)
{
	tic_mem* tic = sfx->tic;

	enum {Width = 73, Height = 83, Round = 2};

	typedef struct {s32 x; s32 y; s32 x1; s32 y1; tic_color color;} Edge; 
	static const Edge Edges[] = 
	{
		{Width, Round, Width, Height - Round, tic_color_15},
		{Round, Height, Width - Round, Height, tic_color_15},
		{Width - Round, Height, Width, Height - Round, tic_color_15},
		{Width - Round, 0, Width, Round, tic_color_15},
		{0, Height - Round, Round, Height, tic_color_15},
		{Round, 0, Width - Round, 0, tic_color_13},
		{0, Round, 0, Height - Round, tic_color_13},
		{0, Round, Round, 0, tic_color_12},
	};

	for(const Edge* edge = Edges; edge < Edges + COUNT_OF(Edges); edge++)
		tic->api.line(tic, x + edge->x, y + edge->y, x + edge->x1, y + edge->y1, edge->color);

	// draw current wave shape
	{
		enum {Scale = 2, MaxValue = (1 << WAVE_VALUE_BITS) - 1};

		tic_rect rect = {x + 5, y + 5, 64, 32};
		tic_sample* effect = getEffect(sfx);
		tic_waveform* wave = getWaveformById(sfx, effect->data->wave);

		drawPanelBorder(tic, rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2, tic_color_5);

		if(checkMousePos(&rect))
		{
			setCursor(tic_cursor_hand);

			s32 cx = (getMouseX() - rect.x) / Scale;
			s32 cy = MaxValue - (getMouseY() - rect.y) / Scale;

			enum {Border = 1};
			tic->api.rect_border(tic, rect.x + cx*Scale - Border, 
				rect.y + (MaxValue - cy) * Scale - Border, Scale + Border*2, Scale + Border*2, tic_color_7);

			if(checkMouseDown(&rect, tic_mouse_left))
			{
				if(tic_tool_peek4(wave->data, cx) != cy)
				{
					tic_tool_poke4(wave->data, cx, cy);
					history_add(sfx->history);
				}
			}
		}		

		for(s32 i = 0; i < WAVE_VALUES; i++)
		{
			s32 value = tic_tool_peek4(wave->data, i);
			tic->api.rect(tic, rect.x + i*Scale, rect.y + (MaxValue - value) * Scale, Scale, Scale, tic_color_7);
		}

		// draw flare
		{
			tic->api.rect(tic, rect.x + 59, rect.y + 2, 4, 1, tic_color_12);
			tic->api.rect(tic, rect.x + 62, rect.y + 2, 1, 3, tic_color_12);			
		}
	}

	drawWaves(sfx, x + 8, y + 43);
}

static void drawPianoOctave(Sfx* sfx, s32 x, s32 y, s32 octave)
{
	tic_mem* tic = sfx->tic;

	enum 
	{
		Gap = 1, WhiteShadow = 1,
		WhiteWidth = 3, WhiteHeight = 8, WhiteCount = 7, WhiteWidthGap = WhiteWidth + Gap,
		BlackWidth = 3, BlackHeight = 4, BlackCount = 6, 
		BlackOffset = WhiteWidth - (BlackWidth - Gap) / 2,
		Width = WhiteCount * WhiteWidthGap - Gap,
		Height = WhiteHeight
	};

	tic_rect rect = {x, y, Width, Height};

	typedef struct{s32 note; tic_rect rect; bool white;} PianoBtn;
	static const PianoBtn Buttons[] = 
	{
		{0, WhiteWidthGap * 0, 0, WhiteWidth, WhiteHeight, true},
		{2, WhiteWidthGap * 1, 0, WhiteWidth, WhiteHeight, true},
		{4, WhiteWidthGap * 2, 0, WhiteWidth, WhiteHeight, true},
		{5, WhiteWidthGap * 3, 0, WhiteWidth, WhiteHeight, true},
		{7, WhiteWidthGap * 4, 0, WhiteWidth, WhiteHeight, true},
		{9, WhiteWidthGap * 5, 0, WhiteWidth, WhiteHeight, true},
		{11, WhiteWidthGap * 6, 0, WhiteWidth, WhiteHeight, true},

		{1, WhiteWidthGap * 0 + BlackOffset, 0, BlackWidth, BlackHeight, false},
		{3, WhiteWidthGap * 1 + BlackOffset, 0, BlackWidth, BlackHeight, false},
		{6, WhiteWidthGap * 3 + BlackOffset, 0, BlackWidth, BlackHeight, false},
		{8, WhiteWidthGap * 4 + BlackOffset, 0, BlackWidth, BlackHeight, false},
		{10, WhiteWidthGap * 5 + BlackOffset, 0, BlackWidth, BlackHeight, false},
	};

	tic_sample* effect = getEffect(sfx);

	if(checkMousePos(&rect))
	{
		for(s32 i = COUNT_OF(Buttons)-1; i >= 0; i--)
		{
			const PianoBtn* btn = Buttons + i;
			tic_rect btnRect = btn->rect;
			btnRect.x += x;
			btnRect.y += y;

			if(checkMousePos(&btnRect))
			{
				setCursor(tic_cursor_hand);

				if(checkMouseDown(&rect, tic_mouse_left))
				{
					effect->note = btn->note;
					effect->octave = octave;
					sfx->play.active = true;

					history_add(sfx->history);
					break;
				}
			}
		}
	}


	bool active = sfx->play.active && effect->octave == octave;

	tic->api.rect(tic, rect.x, rect.y, rect.w, rect.h, tic_color_15);

	for(s32 i = 0; i < COUNT_OF(Buttons); i++)
	{
		const PianoBtn* btn = Buttons + i;
		const tic_rect* rect = &btn->rect;
		tic->api.rect(tic, x + rect->x, y + rect->y, rect->w, rect->h, 
			active && effect->note == btn->note ? tic_color_2 : btn->white ? tic_color_12 : tic_color_0);

		if(btn->white)
			tic->api.rect(tic, x + rect->x, y + (WhiteHeight - WhiteShadow), WhiteWidth, WhiteShadow, tic_color_0);
	}
}

static void drawPiano(Sfx* sfx, s32 x, s32 y)
{
	tic_mem* tic = sfx->tic;

	enum {Width = 29};

	for(s32 i = 0; i < OCTAVES; i++)
	{
		drawPianoOctave(sfx, x + Width*i, y, i);
	}
}

static void tick(Sfx* sfx)
{
	tic_mem* tic = sfx->tic;

	sfx->play.active = false;

	processKeyboard(sfx);
	processEnvelopesKeyboard(sfx);

	tic->api.clear(tic, tic_color_14);

	drawWavePanel(sfx, 10, 21);

	drawCanvas(sfx, 101, 13, SFX_VOLUME_PANEL);
	drawCanvas(sfx, 101, 50, SFX_CHORD_PANEL);
	drawCanvas(sfx, 101, 87, SFX_PITCH_PANEL);

	drawPiano(sfx, 5, 127);

	drawToolbar(tic, true);

	playSound(sfx);
}

static void onStudioEnvelopeEvent(Sfx* sfx, StudioEvent event)
{
	switch(event)
	{
	case TIC_TOOLBAR_CUT: cutToClipboard(sfx); break;
	case TIC_TOOLBAR_COPY: copyToClipboard(sfx); break;
	case TIC_TOOLBAR_PASTE: copyFromClipboard(sfx); break;
	case TIC_TOOLBAR_UNDO: undo(sfx); break;
	case TIC_TOOLBAR_REDO: redo(sfx); break;
	default: break;
	}
}

static void onStudioWaveformEvent(Sfx* sfx, StudioEvent event)
{
	switch(event)
	{
	case TIC_TOOLBAR_CUT: cutWaveToClipboard(sfx); break;
	case TIC_TOOLBAR_COPY: copyWaveToClipboard(sfx); break;
	case TIC_TOOLBAR_PASTE: copyWaveFromClipboard(sfx); break;
	case TIC_TOOLBAR_UNDO: undo(sfx); break;
	case TIC_TOOLBAR_REDO: redo(sfx); break;
	default: break;
	}
}

static void onStudioEvent(Sfx* sfx, StudioEvent event)
{
	onStudioEnvelopeEvent(sfx, event);

	// switch(sfx->tab)
	// {
	// case SFX_WAVEFORM_TAB: onStudioWaveformEvent(sfx, event); break;
	// case SFX_ENVELOPES_TAB: onStudioEnvelopeEvent(sfx, event); break;
	// default: break;
	// }
}

void initSfx(Sfx* sfx, tic_mem* tic, tic_sfx* src)
{
	if(sfx->history) history_delete(sfx->history);
	
	*sfx = (Sfx)
	{
		.tic = tic,
		.tick = tick,
		.src = src,
		.index = 0,
		.play = 
		{
			.note = -1,
			.active = false,
		},

		.history = history_create(src, sizeof(tic_sfx)),
		.event = onStudioEvent,
	};
}
