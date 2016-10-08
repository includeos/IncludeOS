// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SNAKE_HPP
#define SNAKE_HPP

#include <random>
#include <kernel/timers.hpp>

#define S_SIGN '@'
#define S_COLOR ConsoleVGA::vga_color::COLOR_LIGHT_CYAN
#define S_SPAWN Point{{ 35, 12 }}

#define GRID_X 80
#define GRID_Y 25

namespace snake_util
{
using grid_t = int8_t;

class Point
{
public:
	using val_t = grid_t;
	using point_t = std::pair<val_t, val_t>;

	explicit Point(const int);

	explicit Point(const point_t);

	Point(const Point&) = default;
	Point(Point&&) = default;

	Point& operator= (const Point&) = default;
	Point& operator= (Point&&) = default;

	Point operator+ (const Point);
	Point operator* (const Point);

	bool operator== (const Point) const;
	bool operator!= (const Point) const;

	val_t x() const { return _point.first; }
	val_t y() const { return _point.second; }

private:
	point_t _point;
};

struct Part;
}

class Snake
{
public:
	using Point = snake_util::Point;
	using Direction = snake_util::Point;

	explicit Snake(ConsoleVGA&);

	Snake(const Snake&) = delete;
	Snake(Snake&&) = delete;

	Snake& operator= (const Snake&) = delete;
	Snake& operator= (Snake&&) = delete;

	void user_update(const Direction);

	void reset();

	bool finished() const { return !_active; }

private:
	std::vector<snake_util::Part> _body;
	std::vector<snake_util::Part> _food;
	std::vector<snake_util::Part> _obstacles;
	Direction _head_dir;
	ConsoleVGA& _vga;
	bool _active;

	void game_loop();
	void update_positions();
	void intersect();
	void spawn_items();
	void render();
	void gameover();
};

namespace snake_util
{

struct Part
{
	using sign_t = char;
	using color_t = int8_t;

	explicit Part(sign_t, color_t, Point&&);

	sign_t sign;
	color_t color;
	Point pos;
};

}


// ----- IMPLEMENTATION -----


// --- Snake ---

Snake::Snake(ConsoleVGA& vga) :
	_head_dir({ 1, 0 }), _vga(vga), _active(true)
{
	reset();
};

void Snake::user_update(const Direction dir)
{
	auto valid = [&head_dir = _head_dir](const auto dir) -> bool
	{
		return (dir != Point{ { 0, 0 } }
		&& dir != Point{{ -1, -1 }} * head_dir);
	};

	if (valid(dir))
		_head_dir = dir;
}

void Snake::reset()
{
	auto reset_range = [](auto& range) -> void
	{
		range.clear();
		range.reserve(100);
	};
	reset_range(_body);
	reset_range(_food);
	reset_range(_obstacles);

	// spawn head
	_body.emplace_back(S_SIGN, S_COLOR, S_SPAWN);
	_head_dir = Point{{ 1, 0 }};

	spawn_items();

	_active = true;
	game_loop();
}

void Snake::game_loop()
{
	update_positions();
	intersect();
	render();

	if (finished())
	{
		gameover();
		return;
	}

	Timers::oneshot(
		std::chrono::milliseconds(_head_dir.x() == 0 ? 120 : 70),
		[this](auto) { game_loop(); }
	);
}

void Snake::update_positions()
{
	auto head = _body.front();

	std::rotate(
		std::rbegin(_body),
		std::next(std::rbegin(_body)),
		std::rend(_body)
	);

	auto wrap_head_pos = [&point = head.pos]() -> void
	{
		auto wrap = [](auto ic, Point::val_t val) -> Point::val_t
		{
			switch (val)
			{
			case -1: return decltype(ic)::value - 1;
			case decltype(ic)::value: return 0;
			}
			return val;
		};

		std::integral_constant<Point::val_t, GRID_X> x;
		std::integral_constant<Point::val_t, GRID_Y> y;
		point = Point{ { wrap(x, point.x()), wrap(y, point.y()) } };
	};

	head.pos = head.pos + _head_dir;
	wrap_head_pos();
	_body.front() = head;
}

void Snake::intersect()
{
	const auto head = _body.front();

	auto comp = [=](const auto part)
	{
		return head.pos == part.pos;
	};

	auto intersect_range = [=](const auto first, const auto last) -> bool
	{
		return std::none_of(first, last, comp);
	};

	_active = intersect_range(std::next(std::cbegin(_body)), std::cend(_body))
		&& intersect_range(std::cbegin(_obstacles), std::cend(_obstacles));

	auto food_it = std::find_if(std::begin(_food), std::end(_food), comp);
	if (food_it != _food.end())
	{
		_body.insert(std::next(std::cbegin(_body)), std::move(*food_it));
		_food.erase(food_it);

		spawn_items();
	}
}

void Snake::spawn_items()
{
	using val_t = Point::val_t;
	
	static std::mt19937 generator(time(NULL)); // sadly this is ugly
	static std::uniform_int_distribution<val_t> distribution_x(0, GRID_X - 1);
	static std::uniform_int_distribution<val_t> distribution_y(0, GRID_Y - 1);

	auto rand_point = []() -> Point
	{
		return Point{{ distribution_x(generator), distribution_y(generator) }};
	};

	_food.emplace_back('#', ConsoleVGA::vga_color::COLOR_LIGHT_GREEN, rand_point());

	_obstacles.emplace_back('X', ConsoleVGA::vga_color::COLOR_RED, rand_point());
	_obstacles.emplace_back('X', ConsoleVGA::vga_color::COLOR_RED, rand_point());
}

void Snake::render()
{
	_vga.clear();
	auto render_range = [&vga = _vga](const auto& range) -> void
	{
		std::for_each(std::begin(range), std::end(range),
			[&vga](const auto& part) -> void
			{ vga.put(part.sign, part.color, part.pos.x(), part.pos.y()); }
		);
	};

	render_range(_body);
	render_range(_food);
	render_range(_obstacles);
}

void Snake::gameover()
{
	_vga.clear();

	_vga.set_cursor((GRID_X / 5) * 2, GRID_Y / 2 - 1);
	const std::string finished = "GAME OVER ! ! !";
	_vga.write(finished.c_str(), finished.size());

	_vga.set_cursor((GRID_X / 5) * 2, (GRID_Y / 2));
	const std::string finalscore = "SCORE: " + std::to_string(_body.size());
	_vga.write(finalscore.c_str(), finalscore.size());
}


namespace snake_util
{

// --- Point ---

Point::Point(const int key) :
	_point([key]() -> point_t
	{
		switch (key)
		{
		case hw::KBM::VK_UP: return{ 0, -1 };
		case hw::KBM::VK_DOWN: return{ 0, 1 };
		case hw::KBM::VK_RIGHT: return{ 1, 0 };
		case hw::KBM::VK_LEFT: return{ -1, 0 };
		}
		return { 0, 0 };
	}())
{ }

Point::Point(const point_t point)
	: _point(point)
{ }

Point Point::operator+ (const Point other)
{
	return Point{{ _point.first + other._point.first,
		_point.second + other._point.second }};
}

Point Point::operator* (const Point other)
{
	return Point{ { _point.first * other._point.first,
		_point.second * other._point.second } };
}

bool Point::operator== (const Point other) const
{
	return _point == other._point;
}

bool Point::operator!= (const Point other) const
{
	return _point != other._point;
}



// --- Part ---

Part::Part(
	sign_t s,
	const color_t c,
	Point&& p
)
	:
	sign(s),
	color(c),
	pos(std::forward<Point>(p))
{ }

}

#undef S_SIGN
#undef S_COLOR
#undef S_SPAWN

#undef GRID_X
#undef GRID_Y

#endif
