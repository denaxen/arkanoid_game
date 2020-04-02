#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define M_PI 3.1415926536
const float ball_velocity_change = 0.2;
const float paddle_width_decrease = 70;
const float paddle_width_increase = 50;


float operator*(const sf::Vector2f& first, const sf::Vector2f& second)
{
    return first.x*second.x + first.y*second.y;
}

struct Ball;
struct Block;
class Arkanoid;



class Bonus
{
protected:
    static const float speed;
    sf::Vector2f position;
    float time;
    std::string filename;

public:
    Bonus(sf::Vector2f position): position(position)
    {
        time = -100;
    }
// Двигаем бонус
    void update(float dt)
    {
        time += dt;
        position.y += speed * dt;
    }
    void draw(sf::RenderWindow& window, Arkanoid& game);
    virtual void activate(Arkanoid& game){}
    friend class Arkanoid;
};

class Tripple : public Bonus
{
public:
    Tripple(sf::Vector2f position): Bonus (position){filename = "sprites/tripple.png";}
    void activate(Arkanoid& game);
};

class Increase_Speed : public Bonus
{
public:
    Increase_Speed(sf::Vector2f position): Bonus (position){filename = "sprites/increase_speed_sprite.png";}
    void activate(Arkanoid& game);
};

class Decrease_Speed : public Bonus
{
public:
    Decrease_Speed(sf::Vector2f position): Bonus (position){filename = "sprites/decrease_speed_sprite.png";}
    void activate(Arkanoid& game);
};

class Increase_Paddle : public Bonus
{
public:
    Increase_Paddle(sf::Vector2f position): Bonus (position){filename = "sprites/increase_paddle_sprite.png";}
    void activate(Arkanoid& game);
};

class Decrease_Paddle : public Bonus
{
public:
    Decrease_Paddle(sf::Vector2f position): Bonus (position){filename = "sprites/decrease_paddle_sprite.png";}
    void activate(Arkanoid& game);
};

class Fire : public Bonus
{
public:
	Fire(sf::Vector2f position): Bonus (position){filename = "sprites/fire_sprite.png";}
    void activate(Arkanoid& game);
};

class Turrerts : public Bonus
{
public:
	Turrets(sf::Vector2f position): Bonus (position){filename = "sprites/fire_sprite.png";}
    void activate(Arkanoid& game);
};



struct Ball
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    bool is_fired;
};

struct Block
{
    float width, height;
    sf::Vector2f position;
};




class Arkanoid
{
private:
    // Время, которое прошло с начала игры
    float time;
    const float fire_duration = 2.5;
    float fire_time = 0;
    // Границы игрового поля
    float left, right, bottom, top;
    
    // Цвета
    const sf::Color  ball_default_color = sf::Color(246, 213, 92);
    const sf::Color  ball_fired_color = sf::Color(255, 0, 0);
    sf::Color  ball_color;
    sf::Color  paddle_color;
    sf::Color  block_color;

    // Фигуры для рисования, для рисования всех блоков - 1 фигура
    sf::CircleShape ball_shape;
    sf::RectangleShape paddle_shape;
    sf::RectangleShape block_shape;


    float ball_radius;
    float ball_speed;
    // Связыный список всех шариков
    std::list<Ball> balls;

    float block_width, block_height;
    // Связыный список всех блоков
    std::list<Block> blocks;

    // Ракетка
    Block paddle;
    // Переменная, которая показывает находится ли шарик на ракетке
    // В начале игры, или после того, как все шары упали
    bool is_stack;

    // Число жизней
    int number_of_lives;

    // Связыный список указателей на бонусы
    // Почему указатели - для реализации полиформизма
    // Так как в будущем мы хотим сделать несколько вариантов бонусов
    std::list<Bonus*> bonuses;

    // Вероятность того, что при разрушении блока выпадет бонус
    float bonus_probability;



    float norm(sf::Vector2f a)
    {
        return sqrtf(a.x * a.x + a.y * a.y);
    }

    float sqnorm(sf::Vector2f a)
    {
        return a.x * a.x + a.y * a.y;
    }

    // Функция, которая ищет вектор от центра шарика до ближайшей точки блока
    sf::Vector2f find_closest_point(const Ball& ball, const Block& block)
    {
        float rect_left   = block.position.x - block.width/2;
        float rect_right  = block.position.x + block.width/2;
        float rect_bottom = block.position.y - block.height/2;
        float rect_top    = block.position.y + block.height/2;
        sf::Vector2f d;
        if (ball.position.x < rect_left)
            d.x = rect_left;
        else if (ball.position.x > rect_right)
            d.x = rect_right;
        else
            d.x = ball.position.x;

        if (ball.position.y < rect_bottom)
            d.y = rect_bottom;
        else if (ball.position.y > rect_top)
            d.y = rect_top;
        else
            d.y = ball.position.y;

        d -= ball.position;

        return d;
    }

    // Удаляем блок по итературу и создаём бонус с некоторой вероятностью
    void erase_block(std::list<Block>::iterator block_iterator)
    {
        int max_rand = 10000;
        if ((rand() % max_rand) * 1.0f / max_rand < bonus_probability)
        {
        	switch(rand() % 6)
        	{
        		case 0:
        			bonuses.push_back(new Tripple((*block_iterator).position));
        			break;
        		case 1:
        			bonuses.push_back(new Increase_Speed((*block_iterator).position));
        			break;
        		case 2:
        			bonuses.push_back(new Decrease_Speed((*block_iterator).position));
        			break;
        		case 3:
        			bonuses.push_back(new Increase_Paddle((*block_iterator).position));
        			break;
        		case 4:
        			bonuses.push_back(new Decrease_Paddle((*block_iterator).position));
        			break;
        		case 5:
        			bonuses.push_back(new Fire((*block_iterator).position));
        			break;

        	}
            
        }
        blocks.erase(block_iterator);   
    }

    // Обрабатываем столкновения шарика со всеми блоками
    void handle_blocks_collision(Ball& ball)
    {
        // Ищем ближайшую точку до блоков
        // Перебираем все блоки - это не самая эффективная реализация,
        // Можно написать намного быстрее, но не так просто
        sf::Vector2f closest_point = {10000, 10000};
        std::list<Block>::iterator closest_iterator;
        for (std::list<Block>::iterator it = blocks.begin(); it != blocks.end(); it++)
        {
            sf::Vector2f current_closest_point = find_closest_point(ball, *it);
            if (sqnorm(current_closest_point) < sqnorm(closest_point))
            {
                closest_point = current_closest_point;
                closest_iterator = it;
            }
        }

        float closest_point_norm = norm(closest_point);
        // Если расстояние == 0, то это значит, что шарик за 1 фрейм зашёл центром внутрь блока
        // Отражаем шарик от блока
        if (closest_point_norm < 1e-4 && !ball.is_fired)
        {
            if (fabs(ball.velocity.x) > fabs(ball.velocity.y))
                ball.velocity.x *= -1;
            else
                ball.velocity.y *= -1;

            erase_block(closest_iterator);  

        }
        // Если расстояние != 0, но шарик касается блока, то мы можем просчитать отражение более точно
        // Отражение от углов и по касательной.
        else if (sqnorm(closest_point) < ball_radius * ball_radius)
        {
        	if(!ball.is_fired)
        	{
            	ball.position -= closest_point * ((ball_radius - closest_point_norm) / closest_point_norm);
            	ball.velocity -= 2.0f * closest_point * (closest_point * ball.velocity) / (closest_point_norm*closest_point_norm);
            }
            erase_block(closest_iterator);           
        }
    }

    // Обрабатываем столкновения шарика с ракеткой
    void handle_paddle_collision(Ball& ball)
    {
        if (ball.position.y + ball_radius > paddle.position.y - paddle.height/2 &&
            ball.position.y - ball_radius < paddle.position.y + paddle.height/2)
        {
            if (ball.position.x + ball_radius > paddle.position.x - paddle.width/2 &&
                ball.position.x - ball_radius < paddle.position.x + paddle.width/2)
            {
                // Угол отражения зависит от места на ракетке, куда стукнулся шарик
                float velocity_angle = (ball.position.x - paddle.position.x) / (paddle.width + 2 * ball_radius) * (0.8*M_PI) + M_PI/2;
                float velocity_norm = norm(ball.velocity);
                ball.velocity.x = -velocity_norm * cosf(velocity_angle);
                ball.velocity.y = -velocity_norm * sinf(velocity_angle);
            }
        }
    }

    // Обрабатываем столкновения шарика со стенами
    void handle_wall_collision(Ball& ball)
    {
        if (ball.position.x < left + ball_radius)
        {
            ball.position.x = left + ball_radius;
            ball.velocity.x *= -1;
        }
        if (ball.position.x > right - ball_radius)
        {
            ball.position.x = right - ball_radius;
            ball.velocity.x *= -1;
        }
        if (ball.position.y < bottom + ball_radius)
        {
            ball.position.y = bottom + ball_radius;
            ball.velocity.y *= -1;
        }

        /*if (ball.position.y > top - ball_radius)
        {
        	ball.position.y = top - ball_radius;
            ball.velocity.y *= -1;
        }*/

    }

    void handle_all_collisions(Ball& current_ball)
    {
        handle_wall_collision(current_ball);
        handle_blocks_collision(current_ball);
        handle_paddle_collision(current_ball);
    }


    void draw_ball(sf::RenderWindow& window, sf::Vector2f position, sf::Color color)
    {
        ball_shape.setRadius(ball_radius);
        ball_shape.setOrigin(ball_radius, ball_radius);
        ball_shape.setFillColor(color);
        ball_shape.setPosition(position);
        window.draw(ball_shape);
    }




public:
    Arkanoid(const sf::RenderWindow& window)
    {
        time = 0;

        left = 0;
        right = window.getSize().x;
        bottom = 0;
        top = window.getSize().y;

        ball_color = ball_default_color;
        paddle_color = sf::Color::White;
        block_color = sf::Color(100, 200, 250);

        block_width = 40;
        block_height = 16;
        block_shape.setSize({block_width, block_height});

        paddle.width = 120;
        paddle.height = 20;
        paddle.position = {(right - left)/2, top - 100};
        paddle_shape.setSize({paddle.width, paddle.height});
        paddle_shape.setOrigin(paddle.width / 2, paddle.height / 2);

        ball_speed = 600;
        //balls.push_back({{500, 500}, {ball_speed, 400}});
        ball_radius = 8;

        is_stack = true;
        number_of_lives = 5;

        bonus_probability = 0.1;
    }

    void add_block(sf::Vector2f position)
    {
        blocks.push_back({block_width, block_height, position});
    }

    void add_ball(Ball ball)
    {
        balls.push_back(ball);
    }

    // Эта функция вызывается каждый кадр
    void update(const sf::RenderWindow& window, float dt)
    {
        time += dt;
        fire_time += dt;
        // Положение ракетки
        paddle.position.x = sf::Mouse::getPosition(window).x;

        // Обрабатываем шарики
        if (fire_time > fire_duration)
        {
        	for (std::list<Ball>::iterator it = balls.begin(); it != balls.end(); it++)
        	{
        		it->color = ball_default_color;
        		it->is_fired = false;
        	}
        	
        }

        for (std::list<Ball>::iterator it = balls.begin(); it != balls.end(); it++)
        {
            (*it).position += (*it).velocity * dt;
            handle_all_collisions(*it);
            if ((*it).position.y > top - ball_radius)
            {
                it = balls.erase(it);
            }
        }

        // Если шариков нет, то переходи в режим начала игры и уменьшаем кол-во жизней
        if (!is_stack && balls.size() == 0)
        {
            is_stack = true;
            number_of_lives--;
        }
        
        // Обрабатываем бонусы
        for (std::list<Bonus*>::iterator it = bonuses.begin(); it != bonuses.end(); it++)
        {
            (*it)->update(dt);
            if ((*it)->position.y > paddle.position.y - paddle.height/2 && (*it)->position.y < paddle.position.y + paddle.height/2)
            {
                 if (((*it)->position.x > paddle.position.x - paddle.width/2 && (*it)->position.x < paddle.position.x + paddle.width/2))
                 {
                    (*it)->activate(*this);
                    (*it)->time = 0;
                    delete (*it);
                    it = bonuses.erase(it);
                 }
            }
            
        }
    }

    void draw(sf::RenderWindow& window)
    {
        // Рисуем блоки
        for (const Block& block : blocks)
        {
            block_shape.setOrigin(block.width / 2, block.height / 2);
            block_shape.setPosition(block.position);
            block_shape.setFillColor(block_color);
            window.draw(block_shape);
        }

        // Рисуем шарики
        for (const Ball& ball : balls)
        {
            draw_ball(window, ball.position, ball.color);
        }

        // Рисуем ракетку
        paddle_shape.setPosition(paddle.position);
        window.draw(paddle_shape);

        // Если мы в режиме начала игры, то рисуем шарик на ракетке
        if (is_stack)
        {
            draw_ball(window, {paddle.position.x, paddle.position.y - paddle.height/2 - ball_radius}, ball_default_color);
        }

        // Рисуем кол-во жизней вверху слева
        for (int i = 0; i < number_of_lives; i++)
        {
            draw_ball(window, {ball_radius * (3 * i + 2), 2 * ball_radius}, ball_default_color);
        }

        // Рисуем бонусы
        for (Bonus* pbonus : bonuses)
        {
            pbonus->draw(window, *this);
        }
    }


    void on_mouse_pressed(sf::Event& event)
    {
        if (is_stack)
        {
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                is_stack = false;
                float velocity_angle = (rand() % 100 + 40) * M_PI / 180;
                float velocity_norm = ball_speed;
                sf::Vector2f new_ball_position = {paddle.position.x, paddle.position.y - paddle.height/2 - ball_radius};
                sf::Vector2f new_ball_velocity = {-velocity_norm * cosf(velocity_angle), -velocity_norm * sinf(velocity_angle)};
                add_ball({new_ball_position, new_ball_velocity, ball_default_color});
            }
        }
    }



    // Класс бонус должен быть дружественным, так как он может менять внутреннее состояние игры
    friend class Bonus;
    friend class Tripple;
    friend class Increase_Speed;
    friend class Decrease_Speed;
    friend class Increase_Paddle;
    friend class Decrease_Paddle;
    friend class Fire;
    friend class Floor;
    friend class Turrets;
};


void Bonus::draw(sf::RenderWindow& window, Arkanoid& game)
{
	sf::Texture texture;
    texture.loadFromFile(filename);
    sf::Sprite sprite(texture);
    sprite.setScale(0.3, 0.23);
    //sprite.setOrigin(texture.getSize().x/2, texture.getSize().y);
    sprite.setPosition(position);
    window.draw(sprite);
}

void Tripple::activate(Arkanoid& game)
{
    int number_of_balls = game.balls.size();
    std::list<Ball>::iterator ball_it = game.balls.begin();
    for (int i = 0; i < number_of_balls; i++)
    {
        float velocity_x = rand() % ((int)(2*game.ball_speed)) - game.ball_speed;
        float velocity_y = sqrtf(fabs(game.ball_speed*game.ball_speed - velocity_x*velocity_x));
        game.add_ball({(*ball_it).position, {velocity_x, -velocity_y}});

        velocity_x = rand() % ((int)(2*game.ball_speed)) - game.ball_speed;
        velocity_y = sqrtf(fabs(game.ball_speed*game.ball_speed - velocity_x*velocity_x));
        game.add_ball({(*ball_it).position, {velocity_x, -velocity_y}});
        ball_it++;
    }
}


void Increase_Speed::activate(Arkanoid& game)
{
    for (std::list<Ball>::iterator it = game.balls.begin(); it != game.balls.end(); it++)
    	it->velocity = it->velocity + ball_velocity_change * it->velocity;
}

void Decrease_Speed::activate(Arkanoid& game)
{
    for (std::list<Ball>::iterator it = game.balls.begin(); it != game.balls.end(); it++)
    	it->velocity = it->velocity - ball_velocity_change * it->velocity;
}

void Increase_Paddle::activate(Arkanoid& game)
{
	game.paddle.width += paddle_width_increase;
    game.paddle_shape.setSize({game.paddle.width, game.paddle.height});
    game.paddle_shape.setOrigin(game.paddle.width / 2, game.paddle.height / 2);
}

void Decrease_Paddle::activate(Arkanoid& game)
{
    game.paddle.width -= paddle_width_decrease;
    game.paddle_shape.setSize({game.paddle.width, game.paddle.height});
    game.paddle_shape.setOrigin(game.paddle.width / 2, game.paddle.height / 2);
}

void Fire::activate(Arkanoid& game)
{
	game.fire_time = 0;
	game.ball_color = game.ball_fired_color;
	for (std::list<Ball>::iterator it = game.balls.begin(); it != game.balls.end(); it++)
	{
		it->is_fired = true;
		it->color = game.ball_fired_color;
	}
}


// Скорость бонусов
const float Bonus::speed = 100;


int main () 
{
    srand(time(0));
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1000, 800, 32), "Arkanoid", sf::Style::Default, settings);
    window.setFramerateLimit(60);
    
    Arkanoid game(window);
    for(int i = 0; i < 21; ++i)
        for(int j = 0; j < 20; ++j) 
        {
            int stride_x = 40;
            int stride_y = 16;
            game.add_block({(float)((i + 1) * (stride_x + 3) + 22), {(float)((j + 2) * (stride_y +3))}});
        }
        

    while (window.isOpen()) 
    {
        // Обработка событий
        sf::Event event;
        while(window.pollEvent(event)) 
        {
            if(event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) 
            {
                window.close();
            }
            if (event.type == sf::Event::MouseButtonPressed)
            {
                game.on_mouse_pressed(event);
            }
            if (event.type == sf::Event::KeyPressed)
            {
            }


        }
        window.clear(sf::Color(12, 31, 47));
        // Расчитываем новые координаты и новую скорость шарика
        game.update(window, 1.0f/60);
        game.draw(window);

        // Отображам всё нарисованное на временном "холсте" на экран
        window.display();
    }

    return EXIT_SUCCESS;
}
