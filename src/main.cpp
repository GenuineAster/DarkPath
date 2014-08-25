#include <ctime>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <noise/noise.h>

class Game;

struct Entity
{
	enum Type
	{
		Portal,
		Increase
	};

	Type type;
	sf::Vector2f position;
	uint16_t data;
	bool valid=true;
};

class Level
{
friend class Game;
private:
	std::vector<int> m_base;
	std::vector<int> m_mezzanine;
	std::vector<Entity> m_entities;
	int m_seed;
	float m_gap;
	float m_thickness;
	sf::RectangleShape m_tile;
	bool m_portal;
public:
	void create(int seed, int number=1, bool portal=false);
	void draw(sf::RenderTarget& target);
	void increase(int amount);
	float getHeight(float x, bool mezzanine=false, bool predictive=false);
	Entity* getEntity(sf::Vector2f position);
	Level();
};

class Game
{
private:
	sf::View m_view;
	bool m_is_running=true;
	sf::RenderTarget* m_target;
	std::vector<Level> m_levels;
	int m_seed;
	std::map<sf::Keyboard::Key, bool> keyboard;
	struct {
		sf::ConvexShape shape;
		int8_t climb_direction;
	} m_player;
	int m_current_level;

public:
	void setTarget(sf::RenderTarget &target);
	bool processEvent(sf::Event &event);
	bool running();
	void terminate();
	void draw();
	void update(float frametime);
	void generateLevels();

	Game();
};

int main()
{
	Game LD48;
	bool fullscreen=false;
	sf::RenderWindow window{{320, 320}, "Dark Path", sf::Style::Default, sf::ContextSettings{0,0,0,2,0}};
	window.setVerticalSyncEnabled(true);
	LD48.setTarget(window);
	sf::Clock timer;
	while(window.isOpen())
	{
		float frametime = timer.restart().asMicroseconds()/1e6;
		sf::Event event;
		while(window.pollEvent(event))
		{
			if(!LD48.processEvent(event))
			{
				sf::Image screenshot;
				switch(event.type)
				{
				case sf::Event::KeyReleased:
					switch(event.key.code)
					{
					case sf::Keyboard::Escape:
						LD48.terminate();
						window.close();
						break;
					case sf::Keyboard::P:
						screenshot = window.capture();
						if(!screenshot.saveToFile(std::string{"./screenshot-"}+std::to_string(std::time(NULL))+".png"))
							std::cerr<<"Could not save screenshot!"<<std::endl;
						break;
					case sf::Keyboard::J:
						fullscreen=!fullscreen;
						if(fullscreen)
							window.create(sf::VideoMode::getDesktopMode(), "Dark Path", sf::Style::Fullscreen, sf::ContextSettings{0,0,0,2,0});
						else
							window.create({320, 320}, "Dark Path", sf::Style::Default, sf::ContextSettings{0,0,0,2,0});
					default: break;
					}
				}
			}
		}

		window.setTitle(std::string{"Dark Path - FPS: "} + std::to_string((int)(1/frametime)));

		if(!LD48.running())
			window.close();

		LD48.update(frametime);
		window.clear({240,240,240});
		LD48.draw();
		window.display();
	}
	return 0;
}

void Level::create(int seed, int number, bool portal)
{
	m_seed=seed;
	m_portal=portal;

	if(!m_portal)
	{
		noise::module::Perlin perlin;
		perlin.SetSeed(m_seed);
		perlin.SetFrequency(0.5);

		m_base.clear();
		m_mezzanine.clear();
		for(int x=0;x<20;++x)
		{
			m_base.push_back((perlin.GetValue(x/12.34, .0, .0)*128)+128);
			double old_freq = perlin.GetFrequency();
			perlin.SetFrequency(5.0);
			m_mezzanine.push_back((perlin.GetValue(x/12.34, .0, .0)*128)+128);
			perlin.SetFrequency(old_freq);
		}
		m_entities.push_back(Entity());
		m_entities.back().type = Entity::Portal;
		m_entities.back().position = sf::Vector2f(3*-m_thickness, std::min(getHeight(-2*m_thickness), getHeight(-3*m_thickness)));
		m_entities.back().data=0;
		for(int i=0;i<20;++i)
		{
			m_entities.push_back(Entity());
			m_entities.back().type=Entity::Increase;
			if(!(i%9))
			{
				m_entities.back().data = (0 << 4) | 1;
			}
			else
			{
				int lvl = ((seed*number)%(i+1));
				if(lvl==0)
					++lvl;
				m_entities.back().data = (lvl << 8) | std::abs((seed*i)%(100));
			}
			float distance = ((seed/(i+1)*(number+1))%(8*(number+1)));
			float xpos = -m_thickness*distance*(i+1)*(number+1);
			m_entities.back().position = sf::Vector2f{xpos, getHeight(xpos, false, true)};
		}
	}
	else
	{
		for(int x=0;x<5;++x)
		{
			m_base.push_back(128);
			m_mezzanine.push_back(127);
		}
		m_entities.push_back(Entity());
		m_entities.back().type = Entity::Portal;
		m_entities.back().position = sf::Vector2f{-m_thickness*m_base.size()+m_thickness*2, 128.f};
		m_entities.back().data=1;
	}
}

void Level::increase(int amount)
{
	if(!m_portal)
	{
		noise::module::Perlin perlin;
		perlin.SetSeed(m_seed);
		perlin.SetFrequency(0.5);
		//m_base.resize(m_base.size()+amount);

		for(int x=0;x<amount;++x)
		{
			m_base.push_back((perlin.GetValue((m_base.size()+x)/12.34, .0, .0)*128)+128);
			double old_freq = perlin.GetFrequency();
			perlin.SetFrequency(5.0);
			m_mezzanine.push_back((perlin.GetValue((m_mezzanine.size()+x)/12.34, .0, .0)*128)+128);
			perlin.SetFrequency(old_freq);
		}
	}
	else
	{
		for(int x=0;x<5;++x)
		{
			m_base.push_back(128);
			m_mezzanine.push_back(127);
		}
		m_entities.push_back(Entity());
		m_entities.back().type = Entity::Portal;
		m_entities.back().position = sf::Vector2f{-m_thickness*m_base.size()+m_thickness*2, 128.f};
		m_entities.back().data=m_base.size()/5;
	}
}

void Level::draw(sf::RenderTarget& target)
{
	m_tile.setFillColor({190, 190, 190});
	m_tile.setPosition(0.f, 0.f);
	for(auto h : m_mezzanine)
	{
		m_tile.setSize({m_thickness-m_gap, static_cast<float>(h)});
		target.draw(m_tile);
		m_tile.move(-m_thickness, 0.f);
	}
	m_tile.setFillColor(sf::Color::Black);
	m_tile.setPosition(0.f, 0.f);
	for(auto h : m_base)
	{
		m_tile.setSize({m_thickness-m_gap, static_cast<float>(h)});
		target.draw(m_tile);
		m_tile.move(-m_thickness, 0.f);
	}

	sf::RectangleShape ent;
	ent.setSize({30.f, 30.f});
	ent.setOrigin({15.f, 0.f});
	ent.setFillColor(sf::Color::White);
	ent.setOutlineColor(sf::Color::Black);
	ent.setOutlineThickness(5.f);

	int i=0;
	for(auto& e : m_entities)
	{
		if(e.type==Entity::Portal)
		{
			ent.setFillColor(sf::Color::White);
			ent.setPosition(e.position);
			target.draw(ent);
		}
		if(e.type==Entity::Increase)
		{
			if(e.data>>4==0)
			{
				ent.setPosition(e.position);
				ent.setFillColor({50, 50, 50});
			}
			else
			{
				ent.setPosition(e.position);
				ent.setFillColor({150, 150, 150});
			}
			ent.setPosition(ent.getPosition().x, getHeight(ent.getPosition().x, false, true));
			target.draw(ent);
		}
	}
}

float Level::getHeight(float x, bool mezzanine, bool predictive)
{
	if(!predictive)
	{
		if(0>-x/m_thickness+1 || -x/m_thickness+1>=m_base.size())
			return -1;
	}
	else
	{
		if(-x/m_thickness+1 > m_base.size()-1)
		{
			noise::module::Perlin perlin;
			perlin.SetSeed(m_seed);
			if(mezzanine)
			{
				perlin.SetFrequency(5.0);
				return (perlin.GetValue((-x/m_thickness+1)/12.34, .0, .0)*128)+128;
			}
			else
			{
				perlin.SetFrequency(5.0);
				return (perlin.GetValue((-x/m_thickness+1)/12.34, .0, .0)*128)+128;
			}
		}
	}
	if(mezzanine)
		return m_mezzanine[-x/m_thickness+1];
	return m_base[-x/m_thickness+1];
}

Entity* Level::getEntity(sf::Vector2f position)
{
	sf::FloatRect player{position-sf::Vector2f{5.f,7.f}, {10.f, 14.f}};
	for(auto& e:m_entities)
	{
		if(sf::FloatRect{{e.position.x-15.f, (e.position.y==0.f?getHeight(e.position.x):e.position.y)}, {30.f, 30.f}}.intersects(player))//contains(position))
			return &e;
	}
	return nullptr;
}

Level::Level() : m_gap(0.f), m_thickness(24.f)
{
	m_tile.setOutlineColor(sf::Color::White);
	m_tile.setOutlineThickness(m_gap/2.f);
}

void Game::setTarget(sf::RenderTarget &target)
{
	m_target = &target;
	m_view.reset({0.f,0.f,static_cast<float>(m_target->getSize().x),static_cast<float>(m_target->getSize().y)});
	m_view.setRotation(180.f);
	m_view.setCenter(static_cast<sf::Vector2f>(m_target->getSize())/2.f);
}
bool Game::running(){return m_is_running;}
void Game::terminate(){m_is_running=false;return;}
bool Game::processEvent(sf::Event &event)
{
	Entity* e;
	switch(event.type)
	{
	case sf::Event::Closed:
		m_is_running=false;
		return true;
	case sf::Event::KeyReleased:
		keyboard[event.key.code]=false;
		switch(event.key.code)
		{
		case sf::Keyboard::W:
		case sf::Keyboard::Up:
		case sf::Keyboard::Return:
		case sf::Keyboard::Space:
			e = m_levels[m_current_level].getEntity(m_player.shape.getPosition());
			if(e != nullptr)
			{
				if(e->valid)
				{
					if(e->type == Entity::Portal)
					{
						m_current_level = e->data;
						generateLevels();
						m_player.shape.setPosition({-60.f, m_levels[m_current_level].getHeight(-60.f)});
					}
					else if(e->type == Entity::Increase)
					{
						int lvl = (e->data)>>8;
						int amount = (e->data)&0xFF;
						m_levels[lvl].increase((e->data)&0xFF);
						e->valid=false;
					}
				}
			}
			return true;
			break;
		default: return false;
		}
		return false;
	case sf::Event::KeyPressed:
		keyboard[event.key.code]=true;
		return false;
	default: return false;
	}
}

void Game::generateLevels()
{
	for(int i=m_levels.size()-1;i<m_current_level;++i)
	{
		m_levels.emplace_back();
		m_levels.back().create(std::time(NULL), i);
	}
}

void Game::draw()
{
	m_view.setSize(static_cast<sf::Vector2f>(m_target->getSize()));
	m_view.setCenter(m_player.shape.getPosition().x, std::max(m_target->getSize().y/2.f, m_player.shape.getPosition().y));
	m_target->setView(m_view);
	m_levels[m_current_level].draw(*m_target);
	m_target->draw(m_player.shape);
}

void Game::update(float frametime)
{
	sf::Vector2f movement{0.f, 0.f};
	if(keyboard[sf::Keyboard::A] || keyboard[sf::Keyboard::Left])
		movement.x += 100.f*frametime;
	if(keyboard[sf::Keyboard::D] || keyboard[sf::Keyboard::Right])
		movement.x -= 100.f*frametime;

	if(movement.x)
	{
		if(m_player.shape.getPosition().y+std::abs(movement.x) < m_levels[m_current_level].getHeight(m_player.shape.getPosition().x) && 
			(m_player.climb_direction==(std::signbit(movement.x)?-1:1)||m_player.climb_direction==0)
		)
		{
			if(movement.x)
				m_player.climb_direction = (std::signbit(movement.x)?-1:1);
			movement.y = std::abs(movement.x);
			movement.x = 0;
		}
		else
		{
			if(m_player.shape.getPosition().y <= m_levels[m_current_level].getHeight(m_player.shape.getPosition().x))
				m_player.shape.setPosition(m_player.shape.getPosition().x, m_levels[m_current_level].getHeight(m_player.shape.getPosition().x));
			if(m_player.climb_direction!=0)
				m_player.climb_direction = 0;
		}
		m_player.shape.move(movement);
	}
	if(!(m_player.shape.getPosition().y <= m_levels[m_current_level].getHeight(m_player.shape.getPosition().x)))
		m_player.shape.setPosition(m_player.shape.getPosition().x, m_player.shape.getPosition().y-100.f*frametime);
}

Game::Game()
{
	m_view.reset({0.f,0.f,320.f,320.f});
	m_view.setRotation(180.f);
	m_current_level=0;
	m_levels.emplace_back();
	m_levels[m_current_level].create(std::time(NULL), 0, true);
	m_player.shape.setPointCount(3);
	m_player.shape.setPoint(0, {-5,  7});
	m_player.shape.setPoint(1, { 5,  7});
	m_player.shape.setPoint(2, { 0, -7});
	m_player.shape.setOrigin({0, -7});
	m_player.shape.setFillColor(sf::Color::Black);
	m_player.shape.setOutlineThickness(0.5);
	m_player.shape.setOutlineColor({130,130,130});
	m_player.shape.setPosition({0.f, m_levels[m_current_level].getHeight(0.f)});
}
