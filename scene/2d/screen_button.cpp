/*************************************************************************/
/*  screen_button.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2017 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "screen_button.h"
#include "os/os.h"
#include "input_map.h"
#include "os/input.h"

void TouchScreenButton::set_texture(const Ref<Texture>& p_texture) {

	texture=p_texture;
	update();
}

Ref<Texture> TouchScreenButton::get_texture() const{

	return texture;
}

void TouchScreenButton::set_texture_pressed(const Ref<Texture>& p_texture_pressed) {

	texture_pressed=p_texture_pressed;
	update();
}

Ref<Texture> TouchScreenButton::get_texture_pressed() const{

	return texture_pressed;
}

void TouchScreenButton::set_bitmask(const Ref<BitMap>& p_bitmask){

	bitmask=p_bitmask;
}

Ref<BitMap> TouchScreenButton::get_bitmask() const{

	return bitmask;
}

void TouchScreenButton::set_shape(const Ref<Shape2D>& p_shape){

	shape=p_shape;

	if (!is_inside_tree())
		return;
	if (!get_tree()->is_editor_hint() && !get_tree()->is_debugging_collisions_hint())
		return;
	update();
}

Ref<Shape2D> TouchScreenButton::get_shape() const{

	return shape;
}

void TouchScreenButton::set_shape_centered(bool p_shape_centered) {

	shape_centered=p_shape_centered;

	if (!is_inside_tree())
		return;
	if (!get_tree()->is_editor_hint() && !get_tree()->is_debugging_collisions_hint())
		return;
	update();
}

bool TouchScreenButton::is_shape_centered() const {

	return shape_centered;
}

void TouchScreenButton::_notification(int p_what) {

	switch(p_what) {

		case NOTIFICATION_DRAW: {

			if (!is_inside_tree())
				return;
			if (!get_tree()->is_editor_hint() && !OS::get_singleton()->has_touchscreen_ui_hint() && visibility==VISIBILITY_TOUCHSCREEN_ONLY)
				return;

			if (finger_pressed!=-1) {

				if (texture_pressed.is_valid())
					draw_texture(texture_pressed,Point2());
				else if (texture.is_valid())
					draw_texture(texture,Point2());

			} else {
				if (texture.is_valid())
					draw_texture(texture,Point2());
			}

			if (!get_tree()->is_editor_hint() && !get_tree()->is_debugging_collisions_hint())
				return;
			if (shape.is_valid()) {
				Color draw_col=get_tree()->get_debug_collisions_color();
				Vector2 pos=shape_centered ? get_item_rect().size*0.5f : Vector2();
				draw_set_transform_matrix(get_canvas_transform().translated(pos));
				shape->draw(get_canvas_item(),draw_col);
			}

		} break;
		case NOTIFICATION_ENTER_TREE: {

			if (!get_tree()->is_editor_hint() && !OS::get_singleton()->has_touchscreen_ui_hint() && visibility==VISIBILITY_TOUCHSCREEN_ONLY)
				return;
			update();

			if (!get_tree()->is_editor_hint())
				set_process_input(true);

			if (action.operator String()!="" && InputMap::get_singleton()->has_action(action)) {
				action_id=InputMap::get_singleton()->get_action_id(action);
			} else {
				action_id=-1;
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_pressed())
				_release(true);
		} break;
		case NOTIFICATION_PAUSED: {
			// So the button can be pressed again even though the release gets unhandled because of coming during pause
			allow_repress=true;
		}
	}
}


bool TouchScreenButton::is_pressed() const{

	return finger_pressed!=-1;
}

void TouchScreenButton::set_action(const String& p_action) {

	action=p_action;
	if (action.operator String()!="" && InputMap::get_singleton()->has_action(action)) {
		action_id=InputMap::get_singleton()->get_action_id(action);
	} else {
		action_id=-1;
	}

}

String TouchScreenButton::get_action() const {

	return action;
}

void TouchScreenButton::_input(const InputEvent& p_event) {

	if (!get_tree())
		return;

	if (p_event.device != 0)
		return;

	if (passby_press) {

		if (p_event.type==InputEvent::SCREEN_TOUCH && !p_event.screen_touch.pressed && finger_pressed==p_event.screen_touch.index) {

			_release();
		}

		if ((p_event.type==InputEvent::SCREEN_TOUCH && p_event.screen_touch.pressed)|| p_event.type==InputEvent::SCREEN_DRAG) {

			if (finger_pressed==-1 || p_event.screen_touch.index==finger_pressed) {

				Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(Point2(p_event.screen_touch.x,p_event.screen_touch.y));

				bool touched=false;
				if (bitmask.is_valid()) {

					if (Rect2(Point2(),bitmask->get_size()).has_point(coord)) {

						if (bitmask->get_bit(coord))
							touched=true;
					}
				} else {

					if (texture.is_valid())
						touched=Rect2(Point2(),texture->get_size()).has_point(coord);
				}



				if (touched) {
					if (finger_pressed==-1) {
						_press(p_event.screen_touch.index);
					}
				} else {
					if (finger_pressed!=-1) {
						_release();
					}
				}

			}


		}

	} else {

		if (p_event.type==InputEvent::SCREEN_TOUCH) {

			if (p_event.screen_touch.pressed) {

				if (!is_visible_in_tree())
					return;

				const bool can_press=finger_pressed==-1 || allow_repress;
				if (!can_press)
					return; //already fingering

				Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(Point2(p_event.screen_touch.x,p_event.screen_touch.y));
				Rect2 item_rect = get_item_rect();

				bool touched=false;
				bool check_rect=true;
				if (shape.is_valid()) {

					check_rect=false;
					Transform2D xform=shape_centered ? Transform2D().translated(get_item_rect().size*0.5f) : Transform2D();
					touched=shape->collide(xform, unit_rect, Transform2D(0, coord + Vector2(0.5,0.5)));
				}

				if (bitmask.is_valid()) {

					check_rect=false;
					if (!touched && Rect2(Point2(),bitmask->get_size()).has_point(coord)) {

						if (bitmask->get_bit(coord))
							touched=true;
					}
				}

				if (!touched && check_rect) {
					if (!texture.is_null())
						touched=item_rect.has_point(coord);
				}


				if (touched) {
					_press(p_event.screen_touch.index);
				}
			} else {
				if (p_event.screen_touch.index==finger_pressed) {
					_release();
				}
			}
		}
	}
}

void TouchScreenButton::_press(int p_finger_pressed) {

	finger_pressed=p_finger_pressed;
	allow_repress=false;

	if (action_id!=-1) {

		Input::get_singleton()->action_press(action);
		InputEvent ie;
		ie.type=InputEvent::ACTION;
		ie.ID=0;
		ie.action.action=action_id;
		ie.action.pressed=true;
		get_tree()->input_event(ie);
	}

	emit_signal("pressed");
	update();
}

void TouchScreenButton::_release(bool p_exiting_tree) {

	finger_pressed=-1;

	if (action_id!=-1) {

		Input::get_singleton()->action_release(action);
		InputEvent ie;
		ie.type=InputEvent::ACTION;
		ie.ID=0;
		ie.action.action=action_id;
		ie.action.pressed=false;
		get_tree()->input_event(ie);
	}

	if (!p_exiting_tree) {
		emit_signal("released");
		update();
	}
}

Rect2 TouchScreenButton::get_item_rect() const {

	if (texture.is_null())
		return Rect2(0,0,1,1);
	/*
	if (texture.is_null())
		return CanvasItem::get_item_rect();
	*/

	return Rect2(Size2(),texture->get_size());
}


void TouchScreenButton::set_visibility_mode(VisibilityMode p_mode) {
	visibility=p_mode;
	update();
}

TouchScreenButton::VisibilityMode TouchScreenButton::get_visibility_mode() const {

	return visibility;
}

void TouchScreenButton::set_passby_press(bool p_enable) {

	passby_press=p_enable;
}

bool TouchScreenButton::is_passby_press_enabled() const{

	return passby_press;
}



void TouchScreenButton::_bind_methods() {

	ClassDB::bind_method(_MD("set_texture","texture"),&TouchScreenButton::set_texture);
	ClassDB::bind_method(_MD("get_texture"),&TouchScreenButton::get_texture);

	ClassDB::bind_method(_MD("set_texture_pressed","texture_pressed"),&TouchScreenButton::set_texture_pressed);
	ClassDB::bind_method(_MD("get_texture_pressed"),&TouchScreenButton::get_texture_pressed);

	ClassDB::bind_method(_MD("set_bitmask","bitmask"),&TouchScreenButton::set_bitmask);
	ClassDB::bind_method(_MD("get_bitmask"),&TouchScreenButton::get_bitmask);

	ClassDB::bind_method(_MD("set_shape","shape"),&TouchScreenButton::set_shape);
	ClassDB::bind_method(_MD("get_shape"),&TouchScreenButton::get_shape);

	ClassDB::bind_method(_MD("set_shape_centered","bool"),&TouchScreenButton::set_shape_centered);
	ClassDB::bind_method(_MD("is_shape_centered"),&TouchScreenButton::is_shape_centered);

	ClassDB::bind_method(_MD("set_action","action"),&TouchScreenButton::set_action);
	ClassDB::bind_method(_MD("get_action"),&TouchScreenButton::get_action);

	ClassDB::bind_method(_MD("set_visibility_mode","mode"),&TouchScreenButton::set_visibility_mode);
	ClassDB::bind_method(_MD("get_visibility_mode"),&TouchScreenButton::get_visibility_mode);

	ClassDB::bind_method(_MD("set_passby_press","enabled"),&TouchScreenButton::set_passby_press);
	ClassDB::bind_method(_MD("is_passby_press_enabled"),&TouchScreenButton::is_passby_press_enabled);

	ClassDB::bind_method(_MD("is_pressed"),&TouchScreenButton::is_pressed);

	ClassDB::bind_method(_MD("_input"),&TouchScreenButton::_input);

	ADD_PROPERTY( PropertyInfo(Variant::OBJECT,"normal",PROPERTY_HINT_RESOURCE_TYPE,"Texture"),_SCS("set_texture"),_SCS("get_texture"));
	ADD_PROPERTY( PropertyInfo(Variant::OBJECT,"pressed",PROPERTY_HINT_RESOURCE_TYPE,"Texture"),_SCS("set_texture_pressed"),_SCS("get_texture_pressed"));
	ADD_PROPERTY( PropertyInfo(Variant::OBJECT,"bitmask",PROPERTY_HINT_RESOURCE_TYPE,"BitMap"),_SCS("set_bitmask"),_SCS("get_bitmask"));
	ADD_PROPERTY( PropertyInfo(Variant::OBJECT,"shape",PROPERTY_HINT_RESOURCE_TYPE,"Shape2D"),_SCS("set_shape"),_SCS("get_shape"));
	ADD_PROPERTY( PropertyInfo(Variant::BOOL,"shape_centered"),_SCS("set_shape_centered"),_SCS("is_shape_centered"));
	ADD_PROPERTY( PropertyInfo(Variant::BOOL,"passby_press"),_SCS("set_passby_press"),_SCS("is_passby_press_enabled"));
	ADD_PROPERTY( PropertyInfo(Variant::STRING,"action"),_SCS("set_action"),_SCS("get_action"));
	ADD_PROPERTY( PropertyInfo(Variant::INT,"visibility_mode",PROPERTY_HINT_ENUM,"Always,TouchScreen Only"),_SCS("set_visibility_mode"),_SCS("get_visibility_mode"));

	ADD_SIGNAL( MethodInfo("pressed" ) );
	ADD_SIGNAL( MethodInfo("released" ) );



}

TouchScreenButton::TouchScreenButton() {

	finger_pressed=-1;
	allow_repress=false;
	action_id=-1;
	passby_press=false;
	visibility=VISIBILITY_ALWAYS;
	shape_centered=true;
	unit_rect=Ref<RectangleShape2D>(memnew(RectangleShape2D));
	unit_rect->set_extents(Vector2(0.5,0.5));
}
