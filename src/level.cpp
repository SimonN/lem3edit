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
#include "Editor/bar.hpp"
#include "Editor/canvas.hpp"
#include "lem3edit.hpp"
#include "level.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
using namespace std;

void Level::draw(Window * window, signed int x, signed int xOffset, signed int y, signed int yOffset, const Style &style, const Canvas &canvas, int zoom) const
{
	for (int i = 0; i < 3; i++)
	{
		if (canvas.layerVisible[i])
			draw_objects(window, x, xOffset, y, yOffset, i, style, zoom);
	}
}

void Level::draw_objects(Window * window, signed int x, signed int xOffset, signed int y, signed int yOffset, int type, const Style &style, int zoom) const
{
	assert((unsigned)type < COUNTOF(this->object));

	for (vector<Object>::const_iterator i = object[type].begin(); i != object[type].end(); ++i)
	{

		const Object &o = *i;
			
		unsigned int so = style.object_by_id(type, o.id);
		if (so == -1)
			continue;

		int onScreenX = (o.x - x)*zoom - xOffset;
		int onScreenY = (o.y - y)*zoom - yOffset;
		if (onScreenY < window->height - BAR_HEIGHT)
		{
			style.draw_object_texture(window, onScreenX, onScreenY, type, so, zoom, NULL);
		}
	}
}

Level::Object::Index Level::get_object_by_position(signed int x, signed int y, const Style &style, const Canvas &canvas) const
{
	signed int i;

	for (int j = 2; j >= 0; j--)
	{
		if (canvas.layerVisible[j])
		{
			i = get_object_by_position(x, y, j, style);
			if (i != -1)
				return Object::Index(j, i);
		}
	}
	
	return Object::Index(0, -1);
}

signed int Level::get_object_by_position(signed int x, signed int y, int type, const Style &style) const
{
	assert((unsigned)type < COUNTOF(this->object));
	
	for (vector<Object>::const_reverse_iterator i = object[type].rbegin(); i != object[type].rend(); ++i)
	{
		const Object &o = *i;
		
		if (x < o.x || y < o.y)
			continue;
		
		int so = style.object_by_id(type, o.id);
		if (so == -1)
			continue;
		
		if (x < o.x + style.object[type][so].width * 8 && y < o.y + style.object[type][so].height * 2)
			return object[type].rend() - i - 1; // index
	}
	
	return -1;
}

bool Level::load( unsigned int n )
{
	const string path = "LEVELS/";
	const string level = "LEVEL";
	const string perm = "PERM", temp = "TEMP";

	level_id = n;
	
	return load_level(path, level, n) &&
	       load_objects(PERM, path, perm, n) &&
	       load_objects(TEMP, path, temp, n);
}

bool Level::load_level( const std::string &path, const std::string &name, unsigned int n )
{
	const string dat = ".DAT";
	
	return load_level(l3_filename(path, name, n, dat));
}

bool Level::load_level( const string &filename )
{
	ifstream f(filename.c_str(), ios::binary);
	if (!f)
	{
		SDL_Log("Failed to open '%s'\n", filename.c_str());
		return false;
	}
	
	f.read((char *)&tribe,          sizeof(tribe));
	f.read((char *)&cave_map,       sizeof(cave_map));
	f.read((char *)&cave_raw,       sizeof(cave_raw));
	f.read((char *)&temp,           sizeof(temp));
	f.read((char *)&perm,           sizeof(perm));
	f.read((char *)&style,          sizeof(style));
	f.read((char *)&width,          sizeof(width));
	f.read((char *)&height,         sizeof(height));
	f.read((char *)&x,              sizeof(x));
	f.read((char *)&y,              sizeof(y));
	f.read((char *)&time,           sizeof(time));
	f.read((char *)&extra_lemmings, sizeof(extra_lemmings));
	f.read((char *)&unknown,        sizeof(unknown));
	f.read((char *)&release_rate,   sizeof(release_rate));
	f.read((char *)&release_delay,  sizeof(release_delay));
	f.read((char *)&enemies,        sizeof(enemies));
	
	SDL_Log("Loaded level from  '%s'\n", filename.c_str());

	f.close();
	return true;
}

bool Level::load_objects( int type, const string &path, const string &name, unsigned int n )
{
	assert((unsigned)type < COUNTOF(this->object));
	
	const string obs = ".OBS";
	
	return load_objects(type, l3_filename(path, name, n, obs));
}

bool Level::load_objects( int type, const string &filename )
{
	assert((unsigned)type < COUNTOF(this->object));
	
	object[type].clear();
	if (type == PERM)
		object[TOOL].clear();
	
	ifstream f(filename.c_str(), ios::binary);
	if (!f)
	{
		SDL_Log("Failed to open '%s'\n", filename.c_str());
		return false;
	}
	
	while (true)
	{
		Object o;
		
		f.read((char *)&o.id, sizeof(o.id));
		f.read((char *)&o.x,  sizeof(o.x));
		f.read((char *)&o.y,  sizeof(o.y));
		
		if (!f)
			break;

		if (o.id < 5000)
		{
			object[type].push_back(o);
		}
		else
		{
			object[TOOL].push_back(o);
		}
	}
	
	if (type == PERM)
		SDL_Log("Loaded %d + %d objects from '%s'\n", object[type].size(), object[TOOL].size(), filename.c_str());
	if (type == TEMP)
		SDL_Log("Loaded %d objects from '%s'\n", object[type].size(), filename.c_str());
	f.close();
	return true;
}

bool Level::save(unsigned int n)
{
	const string path = "LEVELS/";
	const string level = "LEVEL";
	const string perm = "PERM", temp = "TEMP";

	enemies = 0;
	extra_lemmings = 0;

	return save_objects(PERM, path, perm, n) &&
		save_objects(TEMP, path, temp, n) &&
		save_level(path, level, n);
}

bool Level::save_level(const std::string &path, const std::string &name, unsigned int n)
{
	const string dat = ".DAT";

	return save_level(l3_filename(path, name, n, dat));
}

bool Level::save_level(const string &filename)
{
	ofstream f(filename.c_str(), ios::binary | ios::trunc);
	if (!f)
	{
		SDL_Log("Failed to open '%s'\n", filename.c_str());
		return false;
	}

	f.write((char *)&tribe, sizeof(tribe));
	f.write((char *)&cave_map, sizeof(cave_map));
	f.write((char *)&cave_raw, sizeof(cave_raw));
	f.write((char *)&temp, sizeof(temp));
	f.write((char *)&perm, sizeof(perm));
	f.write((char *)&style, sizeof(style));
	f.write((char *)&width, sizeof(width));
	f.write((char *)&height, sizeof(height));
	f.write((char *)&x, sizeof(x));
	f.write((char *)&y, sizeof(y));
	f.write((char *)&time, sizeof(time));
	f.write((char *)&extra_lemmings, sizeof(extra_lemmings));
	f.write((char *)&unknown, sizeof(unknown));
	f.write((char *)&release_rate, sizeof(release_rate));
	f.write((char *)&release_delay, sizeof(release_delay));
	f.write((char *)&enemies, sizeof(enemies));

	SDL_Log("Wrote level to '%s'\n", filename.c_str());
	f.close();
	return true;
}

bool Level::save_objects(int type, const string &path, const string &name, unsigned int n)
{
	assert((unsigned)type < COUNTOF(this->object));

	const string obs = ".OBS";

	return save_objects(type, l3_filename(path, name, n, obs));
}

bool Level::save_objects(int type, const string &filename)
{
	assert((unsigned)type < COUNTOF(this->object));

	ofstream f(filename.c_str(), ios::binary | ios::trunc);
	if (!f)
	{
		SDL_Log("Failed to open '%s'\n", filename.c_str());
		return false;
	}

	int numTypesToSave = 1;
	int savingType = type;
	if (type == PERM)
		numTypesToSave = 2;
	for (int j = 0; j < numTypesToSave; j++)
	{
		for (vector<Object>::const_iterator i = object[savingType].begin(); i != object[savingType].end(); ++i)
		{
			const Object &o = *i;
			f.write((char *)&o.id, sizeof(o.id));
			f.write((char *)&o.x, sizeof(o.x));
			f.write((char *)&o.y, sizeof(o.y));
			if (o.id == 10006 || o.id == 10007)
				extra_lemmings++;
			if (o.id >= 10010 && o.id <= 10017)
				enemies++;
		}
		savingType = TOOL;
	}
	
	if (type == PERM)
		SDL_Log("Wrote %d + %d objects to '%s'\n", object[type].size(), object[TOOL].size(), filename.c_str());
	if (type == TEMP)
		SDL_Log("Wrote %d objects to '%s'\n", object[type].size(), filename.c_str());
	f.close();
	return true;
}

void Level::resizeLevel(int delta_x, int delta_y, bool shiftLevel)
{
	width += delta_x;
	height += delta_y;
	if (shiftLevel)
	{
		for (vector<Object>::iterator i = object[PERM].begin(); i != object[PERM].end(); ++i)
		{
			Object &o = *i;
			o.x += delta_x;
			o.y += delta_y;
		}
		for (vector<Object>::iterator i = object[TEMP].begin(); i != object[TEMP].end(); ++i)
		{
			Object &o = *i;
			o.x += delta_x;
			o.y += delta_y;
		}
	}
}