#include <stdio.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "LevelCanvas.h"

LevelCanvas::LevelCanvas(int x, int y, int w, int h) 
: BMCanvas(x, y, w, h, "Level View"), _map(0), tc(0), layer(0) {
}

LevelCanvas::~LevelCanvas() {
	if(_map) {
		map_free(_map);
	}
}

int LevelCanvas::handle(int event) {
	switch(event) {
		case FL_RELEASE: {
				take_focus();
					
				tileset *ts = tc->getTileset();
				if(!ts) 
					return 1;
				int tsi = ts_index_of( ts->name );
				if(tsi < 0) 
					return 1;
				
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				int col = mx/_map->tw;
				int row = my/_map->th;					
				
				if(_map && Fl::event_button() == FL_LEFT_MOUSE) {
					int ti = tc->selectedIndex();
					map_set(_map, layer, col, row, tsi, ti);
				} else if(_map && Fl::event_button() == FL_RIGHT_MOUSE) {
					map_set(_map, layer, col, row, tsi, -1);
				}				
				
				redraw();
				return 1;
				
			}break;
		case FL_FOCUS : return 1;
	}	
	return Fl_Widget::handle(event);
}

void LevelCanvas::paint() {
	if(!_map) {
		return;
	}
	
	pen("black");
	clear();
	pen("#808080");
	
	int w = _map->nc * _map->tw;
	int h = _map->nr * _map->th;
	
	for(int j = 0; j < _map->nr; j++) {
		line(0, j * _map->th, w, j * _map->th);
	}
	for(int j = 0; j < _map->nr; j++) {
		line(j * _map->tw, 0, j * _map->tw, h);
	}
	
	for(int i = 0; i < _map->nl; i++)
		map_render(_map, bmp, i, 0, 0);
}

void LevelCanvas::newMap(int nr, int nc, int tw, int th, int nl) {
	if(_map)
		map_free(_map);
	_map = map_create(nr, nc, tw, th, nl);	
	if(!_map) {
		fl_alert("Error creating new map");
	}
	int w = tw * nc;
	int h = th * nr;
	resize(x(), y(), w, h);
}