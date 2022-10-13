#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>
#include <string>
#include <utility>

PlayMode::PlayMode(Client &client_) : client(client_) {
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.downs += 1;
			controls.jump.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_1) {
			controls.oneb.downs += 1;
			controls.oneb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			controls.twob.downs += 1;
			controls.twob.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			controls.threeb.downs += 1;
			controls.threeb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			controls.fourb.downs += 1;
			controls.fourb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_5) {
			controls.fiveb.downs += 1;
			controls.fiveb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_6) {
			controls.sixb.downs += 1;
			controls.sixb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_7) {
			controls.sevenb.downs += 1;
			controls.sevenb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_8) {
			controls.eightb.downs += 1;
			controls.eightb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_9) {
			controls.nineb.downs += 1;
			controls.nineb.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_0) {
			controls.zerob.downs += 1;
			controls.zerob.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_x) {
			controls.xb.downs += 1;
			controls.xb.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_1) {
			controls.oneb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			controls.twob.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			controls.threeb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			controls.fourb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_5) {
			controls.fiveb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_6) {
			controls.sixb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_7) {
			controls.sevenb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_8) {
			controls.eightb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_9) {
			controls.nineb.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_0) {
			controls.zerob.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_x) {
			controls.xb.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	controls.send_controls_message(&client.connection);

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.jump.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
}

glm::u8vec4 tuple_box_col(std::tuple<int, int, int, int> card) {
	switch (std::get<2>(card)) {
		case 0: // first select card
			return glm::u8vec4(0xff, 0x00, 0xff, 0xff);
		case 1: // second select card
			return glm::u8vec4(0xff, 0x00, 0x00, 0xff);
		default: // card not selected
			return glm::u8vec4(0x00, 0x00, 0xff, 0xff);
	}
}

glm::u8vec4 tuple_text_col(std::tuple<int, int, int, int> card) {
	if (std::get<0>(card) < 2) {
		return glm::u8vec4(0xff, 0x00, 0x00, 0xff);
	} else {
		return glm::u8vec4(0xff, 0xff, 0xff, 0xff);
	}
}

std::string tuple_to_string(std::tuple<int, int, int, int> card) {
	std::string res = "";
	switch(std::get<0>(card)) {
		case 0:
			res += "D";
			break;
		case 1:
			res += "H";
			break;
		case 2:
			res += "C";
			break;
		default:
			res += "S";
			break;
	}
	res += " ";
	switch (std::get<1>(card)) {
		case 12:
			res += "K";
			break;
		case 11:
			res += "Q";
			break;
		case 10:
			res += "J";
			break;
		case 0:
			res += "A";
			break;
		case -1:
			break;
		default:
			res += std::to_string(std::get<1>(card) + 1);
			break;
	}
	return res;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	//figure out view transform to center the arena:
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	float scale = std::min(
		2.0f * aspect / (Game::ArenaMax.x - Game::ArenaMin.x + 2.0f * Game::PlayerRadius),
		2.0f / (Game::ArenaMax.y - Game::ArenaMin.y + 2.0f * Game::PlayerRadius)
	);
	glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

	glm::mat4 world_to_clip = glm::mat4(
		scale / aspect, 0.0f, 0.0f, offset.x,
		0.0f, scale, 0.0f, offset.y,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	{
		DrawLines lines(world_to_clip);

		//helper:
		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H, glm::u8vec4 col) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				col);
			float ofs = (1.0f / scale) / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				col);
		};

		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, (Game::ArenaMax.y - Game::ArenaMin.y) / 2.0f, 0.0f), glm::vec3(Game::ArenaMax.x, (Game::ArenaMax.y - Game::ArenaMin.y) / 2.0f, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));

		lines.draw_quad(Game::ArenaMin.x, Game::ArenaMin.y, Game::ArenaMax.x, Game::ArenaMax.y, glm::u8vec4(0x00, 0x00, 0xff, 0xff));

		float forthW = (Game::ArenaMax.x - Game::ArenaMin.x - 0.1f * 6.0f) / 5.0;

		bool done = false;
		for (auto const &player : game.players) {
			if (player.done) {
				done = true;
			}
		}

		for (auto const &player : game.players) {
			glm::u8vec4 col = glm::u8vec4(player.color.x*255, player.color.y*255, player.color.z*255, 0xff);
			if (&player == &game.players.front()) {
				// Draw neg_pile
				if (player.neg_pile.size() > 0) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMin.y + 0.1f + 0.05f * 12, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMin.y + 0.1f + 0.05f * 13, tuple_box_col(game.players.front().neg_pile.back()));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMin.y + 0.1f + 0.05f * 12), tuple_to_string(player.neg_pile.back()), 0.04f, tuple_text_col(game.players.front().neg_pile.back()));
				} else {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMin.y + 0.1f + 0.05f * 12, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMin.y + 0.1f + 0.05f * 13, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMin.y + 0.1f + 0.05f * 12), "end", 0.04f, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
				}
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMin.y + 0.1f + 0.05f * 11), std::to_string(game.players.front().neg_pile.size()), 0.04f, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
				
				// Draw pos_pile
				if (player.pos_pile.size() > 0) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMin.y + 0.1f, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMin.y + 0.1f + 0.05f, tuple_box_col(player.pos_pile.at(player.pos_pos)));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMin.y + 0.1f), tuple_to_string(player.pos_pile.at(player.pos_pos)), 0.04f, tuple_text_col(player.pos_pile.at(player.pos_pos)));
				} else {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMin.y + 0.1f, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMin.y + 0.1f + 0.05f, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMin.y + 0.1f), "none", 0.04f, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
				}

				// Draw active piles
				for (int i = 0; i < player.one.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * (13 - i), tuple_box_col(player.one.at(i)));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i)), tuple_to_string(player.one.at(i)), 0.04f, tuple_text_col(player.one.at(i)));
				}
				for (int i = 0; i < player.two.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * (13 - i), tuple_box_col(player.two.at(i)));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i)), tuple_to_string(player.two.at(i)), 0.04f, tuple_text_col(player.two.at(i)));
				}
				for (int i = 0; i < player.three.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * (13 - i), tuple_box_col(player.three.at(i)));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i)), tuple_to_string(player.three.at(i)), 0.04f, tuple_text_col(player.three.at(i)));
				}
				for (int i = 0; i < player.four.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * (13 - i), tuple_box_col(player.four.at(i)));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * (12 - i)), tuple_to_string(player.four.at(i)), 0.04f, tuple_text_col(player.four.at(i)));
				}

				// Draw suit piles
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * 16, tuple_box_col(player.D));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 1, Game::ArenaMin.y + 0.1f + 0.05f * 15), tuple_to_string(player.D), 0.04f, tuple_text_col(player.D));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * 16, tuple_box_col(player.H));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 2, Game::ArenaMin.y + 0.1f + 0.05f * 15), tuple_to_string(player.H), 0.04f, tuple_text_col(player.H));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * 16, tuple_box_col(player.C));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 3, Game::ArenaMin.y + 0.1f + 0.05f * 15), tuple_to_string(player.C), 0.04f, tuple_text_col(player.C));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * 16, tuple_box_col(player.S));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 4, Game::ArenaMin.y + 0.1f + 0.05f * 15), tuple_to_string(player.S), 0.04f, tuple_text_col(player.S));

			} else {
				// Draw neg_pile
				if (player.neg_pile.size() > 0) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMax.y - 0.1f - 0.05f * 12, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMax.y - 0.1f - 0.05f * 13, glm::u8vec4(0x00, 0x00, 0xff, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMax.y - 0.1f - 0.05f * 13), tuple_to_string(player.neg_pile.back()), 0.04f, tuple_text_col(player.neg_pile.back()));
				} else {
					lines.draw_quad(Game::ArenaMin.x + 0.1f, Game::ArenaMax.y - 0.1f - 0.05f * 12, Game::ArenaMin.x + 0.1f + forthW, Game::ArenaMax.y - 0.1f - 0.05f * 13, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMax.y - 0.1f - 0.05f * 13), "end", 0.04f, glm::u8vec4(0xff, 0x00, 0x00, 0xff));
				}
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMax.y - 0.1f - 0.05f * 12), std::to_string(player.neg_pile.size()), 0.04f, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
				
				// Draw score if applicable -- don't care about other player's pos_pile
				if (done) {
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f, Game::ArenaMax.y - 0.1f - 0.05f), std::to_string(game.players.front().score) + " vs. " + std::to_string(player.score), 0.04f, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
				}

				// Draw active piles
				for (int i = 0; i < player.one.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i)), tuple_to_string(player.one.at(i)), 0.04f, tuple_text_col(player.one.at(i)));
				}
				for (int i = 0; i < player.two.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i)), tuple_to_string(player.two.at(i)), 0.04f, tuple_text_col(player.two.at(i)));
				}
				for (int i = 0; i < player.three.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i)), tuple_to_string(player.three.at(i)), 0.04f, tuple_text_col(player.three.at(i)));
				}
				for (int i = 0; i < player.four.size(); i++) {
					lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * (12 - i), Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
					draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * (13 - i)), tuple_to_string(player.four.at(i)), 0.04f, tuple_text_col(player.four.at(i)));
				}

				// Draw suit piles
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * 16, glm::u8vec4(0x00, 0x00, 0xff, 0xff));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 1, Game::ArenaMax.y - 0.1f - 0.05f * 16), tuple_to_string(player.D), 0.04f, tuple_text_col(player.D));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * 16, glm::u8vec4(0x00, 0x00, 0xff, 0xff));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 2, Game::ArenaMax.y - 0.1f - 0.05f * 16), tuple_to_string(player.H), 0.04f, tuple_text_col(player.H));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * 16, glm::u8vec4(0x00, 0x00, 0xff, 0xff));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 3, Game::ArenaMax.y - 0.1f - 0.05f * 16), tuple_to_string(player.C), 0.04f, tuple_text_col(player.C));
				lines.draw_quad(Game::ArenaMin.x + 0.1f + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * 15, Game::ArenaMin.x + 0.1f + forthW + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * 16, glm::u8vec4(0x00, 0x00, 0xff, 0xff));
				draw_text(glm::vec2(Game::ArenaMin.x + 0.1f + 0.075f + (forthW + 0.1f) * 4, Game::ArenaMax.y - 0.1f - 0.05f * 16), tuple_to_string(player.S), 0.04f, tuple_text_col(player.S));
			}
		}
	}
	GL_ERRORS();
}
