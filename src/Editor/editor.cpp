/*
 * lem3edit
 * Copyright (C) 2008-2009 Carl Reinke
 * Copyright (C) 2017 Kieran Millar
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
This file is the top level object of the editor and handles some general purpose functionality
*/
#include "editor.hpp"

#include <cassert>
#include <iostream>
using namespace std;

Editor::Editor(void)
{
	font.load("FONT");
}

void Editor::resize( int w, int h)
{
	canvas.resize(h);
	bar.resizeBarScrollRect(w, h);
}

bool Editor::load( int n, Window * w )
{
	window_ptr = w;
	bar.setReferences(window_ptr, this, &canvas, &style);
	canvas.setReferences(window_ptr, this, &editor_input, &bar, &style, &level);
	editor_input.setReferences(window_ptr, this, &bar, &canvas, &style, &level);
	level.load(n);
	tribe.load(level.tribe);
	style.load(level.style, window_ptr, tribe.palette);
	bar.load();
	canvas.load();
	editor_input.load();
	gameFrameCount = 0;
	gameFrameTick = SDL_GetTicks();
	
	return canvas.redraw = true;
}

bool Editor::save(int n)
{
	level.save(n);

	return canvas.redraw = true;
}

bool Editor::select( signed int x, signed int y, bool modify_selection )
{
	Level::Object::Index temp = level.get_object_by_position(x, y, style, canvas.backgroundOnly);
	
	if (temp.i == -1)  // selected nothing
	{
		selection.clear();
		canvas.redraw = true;
		return false;
	}
	else
	{
		bool already_selected = selection.find(temp) != selection.end();
		if (already_selected)
		{
			if (modify_selection)
				selection.erase(temp);
			else
			{
				editor_input.dragging = true;
				editor_input.startDragTime = gameFrameCount;
			}
		}
		else
		{
			if (modify_selection)
				selection.insert(temp);
			else
			{
				selection.clear();
				selection.insert(temp);
				editor_input.dragging = true;
			}
		}
		canvas.redraw = true;
		return true;
	}
}

bool Editor::select_none( void )
{
	if (selection.empty())
		return false;
	
	selection.clear();
	
	return canvas.redraw = true;
}

bool Editor::select_all( void )
{
	for (unsigned int type = 0; type < COUNTOF(level.object); ++type)
	{
		for (unsigned int i = 0; i < level.object[type].size(); ++i)
		{
			selection.insert(Level::Object::Index(type, i));
		}
	}
	
	return canvas.redraw = !selection.empty();
}

bool Editor::copy_selected( void )
{
	clipboard.clear();
	
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		clipboard.push_back(pair<Level::Object::Index, Level::Object>(*i, level.object[i->type][i->i]));
	}
	
	return !clipboard.empty();
}

bool Editor::paste( void )
{
	selection.clear(); // maybe delete selection instead?
	
	for (Clipboard::const_iterator i = clipboard.begin(); i != clipboard.end(); ++i)
	{		
		const Level::Object::Index &index = i->first;
		
		level.object[index.type].push_back(i->second);
		
		selection.insert(Level::Object::Index(index.type, level.object[index.type].size() - 1));
	}
	
	return canvas.redraw = !clipboard.empty();
}

bool Editor::addObject(int idToAdd, int typeToAdd, int xToAdd, int yToAdd)
{
	Level::Object o;

	o.id = idToAdd;
	o.x = xToAdd;
	o.y = yToAdd;
	level.object[typeToAdd].push_back(o);

	return canvas.redraw = true;
}

bool Editor::moveToFront(void)
{
/*	int selectionSize = selection.size();
	std::vector<Level::Object> copiedObjects[2] {level.object[0], level.object[1]};

	int count = 0;
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		Level::Object &o = level.object[i->type][i->i];
		Level::Object newObj = o;

		copiedObjects[i->type].erase(copiedObjects[i->type].begin() + i->i + count);
		copiedObjects[i->type].push_back(newObj);

		level.object[0] = copiedObjects[0];
		level.object[1] = copiedObjects[1];

		count--;
	}

	selection.clear();*/

	return canvas.redraw = true;
}

bool Editor::moveToBack(void)
{
/*	int selectionSize = selection.size();
	std::vector<Level::Object> copiedObjects[2] {level.object[0], level.object[1]};

	int count = 0;
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		Level::Object &o = level.object[i->type][i->i];
		Level::Object newObj = o;

		copiedObjects[i->type].erase(copiedObjects[i->type].begin() + i->i + count);
		copiedObjects[i->type].insert(copiedObjects[i->type].begin(), newObj);

		level.object[0] = copiedObjects[0];
		level.object[1] = copiedObjects[1];

		count++;
	}

	selection.clear();*/

	return canvas.redraw = true;
}

bool Editor::decrease_obj_id( void )
{
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		Level::Object &o = level.object[i->type][i->i];
		o.id = style.object_prev_id(i->type, o.id);
	}

	return canvas.redraw = true;
}

bool Editor::increase_obj_id( void )
{
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		Level::Object &o = level.object[i->type][i->i];
		o.id = style.object_next_id(i->type, o.id);
	}

	return canvas.redraw = true;
}

bool Editor::delete_selected( void )
{
	if (selection.empty())
		return false;
	
	for (Selection::const_reverse_iterator i = selection.rbegin(); i != selection.rend(); ++i)
		level.object[i->type].erase(level.object[i->type].begin() + i->i);
	
	selection.clear();
	
	return canvas.redraw = true;
}

//This function takes in how much the mouse has moved in the last frame
//Input is dependent of zoom, so provide screen pixel values
//So to ensure smooth scrolling, it maintains a remainder after relocating the moved objects
//and snapping them to the grid
bool Editor::move_selected( signed int delta_x, signed int delta_y )
{
	delta_x += canvas.mouse_remainder_x;
	delta_y += canvas.mouse_remainder_y;
	canvas.mouse_remainder_x = delta_x % (8 * canvas.zoom);
	canvas.mouse_remainder_y = delta_y % (2 * canvas.zoom);

	delta_x /= (8 * canvas.zoom);
	delta_y /= (2 * canvas.zoom);
	
	if (selection.empty() || (delta_x == 0 && delta_y == 0))
		return false;
	
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		Level::Object &o = level.object[i->type][i->i];
		o.x += delta_x*8;
		o.y += delta_y*2;
	}
	
	return canvas.redraw = true;
}

bool Editor::move_selected_z( signed int delta_z )
{
	if (selection.empty())
		return false;
	
	for (Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
	{
		assert(false); // TODO
		(void)delta_z;
	}
	
	return canvas.redraw = true;
}