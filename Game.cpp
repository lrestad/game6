#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

#include <glm/gtx/norm.hpp>

#include <array>        // std::array
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock

void Player::Controls::send_controls_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 16;
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(jump);
	send_button(oneb);
	send_button(twob);
	send_button(threeb);
	send_button(fourb);
	send_button(fiveb);
	send_button(sixb);
	send_button(sevenb);
	send_button(eightb);
	send_button(nineb);
	send_button(zerob);
	send_button(xb);
}

bool Player::Controls::recv_controls_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 16) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			d = 255;
		}
		button->downs = uint8_t(d);
	};

	recv_button(recv_buffer[4+0], &left);
	recv_button(recv_buffer[4+1], &right);
	recv_button(recv_buffer[4+2], &up);
	recv_button(recv_buffer[4+3], &down);
	recv_button(recv_buffer[4+4], &jump);
	recv_button(recv_buffer[4+5], &oneb);
	recv_button(recv_buffer[4+6], &twob);
	recv_button(recv_buffer[4+7], &threeb);
	recv_button(recv_buffer[4+8], &fourb);
	recv_button(recv_buffer[4+9], &fiveb);
	recv_button(recv_buffer[4+10], &sixb);
	recv_button(recv_buffer[4+11], &sevenb);
	recv_button(recv_buffer[4+12], &eightb);
	recv_button(recv_buffer[4+13], &nineb);
	recv_button(recv_buffer[4+14], &zerob);
	recv_button(recv_buffer[4+15], &xb);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}


//-----------------------------------------

Game::Game() : mt(0x15466666) {
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	int player_num = next_player_number++;
	player.name = "Player " + std::to_string(player_num);

	// Make player deck and shuffle
	std::vector<std::tuple<int, int, int, int>> pos_pile;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 13; j++) {
			pos_pile.push_back(std::make_tuple(i, j, 2, player_num));
		}
	}
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::shuffle (pos_pile.begin(), pos_pile.end(), std::default_random_engine(seed));

	// Make the neg_pile and allocate 13 cards
	std::vector<std::tuple<int, int, int, int>> neg_pile;
	for (int i = 0; i < 13; i++) {
		neg_pile.push_back(pos_pile.back());
		pos_pile.pop_back();
	}

	// Make the active piles and allocate 1 card each
	std::vector<std::tuple<int, int, int, int>> one;
	one.push_back(pos_pile.back());
	pos_pile.pop_back();
	std::vector<std::tuple<int, int, int, int>> two;
	two.push_back(pos_pile.back());
	pos_pile.pop_back();
	std::vector<std::tuple<int, int, int, int>> three;
	three.push_back(pos_pile.back());
	pos_pile.pop_back();
	std::vector<std::tuple<int, int, int, int>> four;
	four.push_back(pos_pile.back());
	pos_pile.pop_back();

	// Set suit piles
	std::tuple<int, int, int, int> D = std::make_tuple(0, -1, 2, player_num);
	std::tuple<int, int, int, int> H = std::make_tuple(1, -1, 2, player_num);
	std::tuple<int, int, int, int> C = std::make_tuple(2, -1, 2, player_num);
	std::tuple<int, int, int, int> S = std::make_tuple(3, -1, 2, player_num);

	// positive score
	int score = 0;

	// Set piles
	player.pos_pile = pos_pile;
	player.neg_pile = neg_pile;
	player.one = one;
	player.two = two;
	player.three = three;
	player.four = four;
	player.D = D;
	player.H = H;
	player.C = C;
	player.S = S;
	player.score = score;
	player.pos_pos = 3;

	return &player;
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

void Game::update(float elapsed) {

	bool done = false;
	for (auto &p : players) {
		if (p.controls.xb.pressed) {
			p.done = true;
		}
		done = done || p.done;
	}
	if (done) {
		return;
	}

	for (auto &p : players) {
		// controls the current top of the pos_pile
		if (p.controls.jump.pressed) {
			p.pos_pos += 3;
			if (p.pos_pos >= p.pos_pile.size()) {
				p.pos_pos = 3;
			}
			if (p.pos_pos >= p.pos_pile.size()) {
				p.pos_pos = p.pos_pile.size() - 1;
			}
		}

		// controls the neg_pile -- can move from not to
		if (p.controls.oneb.pressed) {
			if (p.first == -1 && p.neg_pile.size() > 0) {
				p.first = 1;
				std::tuple<int, int, int, int> temp = p.neg_pile.back();
				p.neg_pile.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.first == 1) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.neg_pile.back();
				p.neg_pile.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		// controls the one pile -- can move to and from
		if (p.controls.twob.pressed) {
			if (p.first == -1) {
				p.first = 2;
				std::tuple<int, int, int, int> temp = p.one.back();
				p.one.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.second == -1 && p.first != 2) {
				p.second = 2;
			} else if (p.first == 2) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.one.back();
				p.one.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		// controls the two pile -- can move to and from
		if (p.controls.threeb.pressed) {
			if (p.first == -1) {
				p.first = 3;
				std::tuple<int, int, int, int> temp = p.two.back();
				p.two.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.second == -1 && p.first != 2) {
				p.second = 3;
			} else if (p.first == 3) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.two.back();
				p.two.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		// controls the three pile -- can move to and from
		if (p.controls.fourb.pressed) {
			if (p.first == -1) {
				p.first = 4;
				std::tuple<int, int, int, int> temp = p.three.back();
				p.three.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.second == -1 && p.first != 4) {
				p.second = 4;
			} else if (p.first == 4) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.three.back();
				p.three.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		// controls the four pile -- can move to and from
		if (p.controls.fiveb.pressed) {
			if (p.first == -1) {
				p.first = 5;
				std::tuple<int, int, int, int> temp = p.four.back();
				p.four.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.second == -1 && p.first != 5) {
				p.second = 5;
			} else if (p.first == 5) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.four.back();
				p.four.back() = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		// controls the D pile -- can move to not from
		if (p.controls.sixb.pressed) {
			if (p.first != -1 && p.second == -1) {
				p.second = 6;
			}
		}

		// controls the H pile -- can move to not from
		if (p.controls.sevenb.pressed) {
			if (p.first != -1 && p.second == -1) {
				p.second = 7;
			}
		}

		// controls the C pile -- can move to not from
		if (p.controls.eightb.pressed) {
			if (p.first != -1 && p.second == -1) {
				p.second = 8;
			}
		}

		// controls the S pile -- can move to not from
		if (p.controls.nineb.pressed) {
			if (p.first != -1 && p.second == -1) {
				p.second = 9;
			}
		}

		// controls the pos_pile -- can move from not to
		if (p.controls.zerob.pressed) {
			if (p.first == -1 && p.neg_pile.size() > 0) {
				p.first = 0;
				std::tuple<int, int, int, int> temp = p.pos_pile.at(p.pos_pos);
				p.pos_pile.at(p.pos_pos) = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 0, std::get<3>(temp));
			} else if (p.first == 0) {
				p.first = -1;
				std::tuple<int, int, int, int> temp = p.pos_pile.at(p.pos_pos);
				p.pos_pile.at(p.pos_pos) = std::make_tuple(std::get<0>(temp), std::get<1>(temp), 2, std::get<3>(temp));
			}
		}

		if (p.first >= 0 && p.second >= 0) {
			bool success = false;
			std::tuple<int, int, int, int> move_card;
			switch (p.first) {
				// can be neg_pile, pos_pile, one, two, three, or four
				case 0: // pos_pile
					move_card = p.pos_pile.at(p.pos_pos);
					break;
				case 1: // neg_pile
					move_card = p.neg_pile.back();
					break;
				case 2: // one
					move_card = p.one.back();
					break;
				case 3: // two
					move_card = p.two.back();
					break;
				case 4: // three
					move_card = p.three.back();
					break;
				default: // four
					move_card = p.four.back();
					break;
			}

			std::tuple<int, int, int, int> new_card = std::make_tuple(std::get<0>(move_card), std::get<1>(move_card), 2, std::get<2>(move_card));
			
			// can be one, two, three, four, D, H, C, or S
			switch (p.second) {
				// next four: alternating color, decrease by one
				case 2:
					if (p.one.size() == 0) {
						success = true;
						p.one.push_back(new_card);
					} else if (((std::get<0>(p.one.back()) < 2 && std::get<0>(new_card) >= 2) || (std::get<0>(p.one.back()) >= 2 && std::get<0>(new_card) < 2)) && (std::get<1>(p.one.back()) == std::get<1>(new_card) + 1)) {
						success = true;
						p.one.push_back(new_card);
					}
					break;
				case 3:
					if (p.two.size() == 0) {
						success = true;
						p.two.push_back(new_card);
					} else if (((std::get<0>(p.two.back()) < 2 && std::get<0>(new_card) >= 2) || (std::get<0>(p.two.back()) >= 2 && std::get<0>(new_card) < 2)) && (std::get<1>(p.two.back()) == std::get<1>(new_card) + 1)) {
						success = true;
						p.two.push_back(new_card);
					}
					break;
				case 4:
					if (p.three.size() == 0) {
						success = true;
						p.three.push_back(new_card);
					} else if (((std::get<0>(p.three.back()) < 2 && std::get<0>(new_card) >= 2) || (std::get<0>(p.three.back()) >= 2 && std::get<0>(new_card) < 2)) && (std::get<1>(p.three.back()) == std::get<1>(new_card) + 1)) {
						success = true;
						p.three.push_back(new_card);
					}
					break;
				case 5:
					if (p.four.size() == 0) {
						success = true;
						p.four.push_back(new_card);
					} else if (((std::get<0>(p.four.back()) < 2 && std::get<0>(new_card) >= 2) || (std::get<0>(p.four.back()) >= 2 && std::get<0>(new_card) < 2)) && (std::get<1>(p.four.back()) == std::get<1>(new_card) + 1)) {
						success = true;
						p.four.push_back(new_card);
					}
					break;
				
				// next four: same suit, increase by one
				case 6:
					if (std::get<0>(new_card) == std::get<0>(p.D) && std::get<1>(new_card) == std::get<1>(p.D) + 1) {
						success = true;
						p.D = new_card;
						p.score++;
					}
					break;
				case 7:
					if (std::get<0>(new_card) == std::get<0>(p.H) && std::get<1>(new_card) == std::get<1>(p.H) + 1) {
						success = true;
						p.H = new_card;
						p.score++;
					}
					break;
				case 8:
					if (std::get<0>(new_card) == std::get<0>(p.C) && std::get<1>(new_card) == std::get<1>(p.C) + 1) {
						success = true;
						p.C = new_card;
						p.score++;
					} 
					break;
				default:
					if (std::get<0>(new_card) == std::get<0>(p.S) && std::get<1>(new_card) == std::get<1>(p.S) + 1) {
						success = true;
						p.S = new_card;
						p.score++;
					} 
					break;
			}

			// need to remove
			if (success) {
				// remove
				switch (p.first) {
					// can be neg_pile, pos_pile, one, two, three, or four
					case 0: // pos_pile
						p.pos_pile.erase(p.pos_pile.begin()+p.pos_pos);
						if (p.pos_pos != 0) {
							p.pos_pos--;
						}
						break;
					case 1: // neg_pile
						p.neg_pile.pop_back();
						break;
					case 2: // one
						p.one.pop_back();
						break;
					case 3: // two
						p.two.pop_back();
						break;
					case 4: // three
						p.three.pop_back();
						break;
					default: // four
						p.four.pop_back();
						break;
				}
			} else {
				// fix coloring
				switch (p.first) {
					// can be neg_pile, pos_pile, one, two, three, or four
					case 0: // pos_pile
						p.pos_pile.at(p.pos_pos) = new_card;
						break;
					case 1: // neg_pile
						p.neg_pile.back() = new_card;
						break;
					case 2: // one
						p.one.back() = new_card;
						break;
					case 3: // two
						p.two.back() = new_card;
						break;
					case 4: // three
						p.three.back() = new_card;
						break;
					default: // four
						p.four.back() = new_card;
						break;
				}
			}

			if (p.neg_pile.size() == 0) {
				p.done = true;
			}

			p.first = -1;
			p.second = -1;
		}

		//reset 'downs' since controls have been handled:
		p.controls.left.downs = 0;
		p.controls.right.downs = 0;
		p.controls.up.downs = 0;
		p.controls.down.downs = 0;
		p.controls.jump.downs = 0;
		p.controls.oneb.downs = 0;
		p.controls.twob.downs = 0;
		p.controls.threeb.downs = 0;
		p.controls.fourb.downs = 0;
		p.controls.fiveb.downs = 0;
		p.controls.sixb.downs = 0;
		p.controls.sevenb.downs = 0;
		p.controls.eightb.downs = 0;
		p.controls.nineb.downs = 0;
		p.controls.zerob.downs = 0;
		p.controls.xb.downs = 0;
	}
}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer


	//send player info helper:
	auto send_player = [&](Player const &player) {
		connection.send(player.position);
		connection.send(player.velocity);
		connection.send(player.color);
		//connection.send(player.neg_pile);
	
		//NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
		//effectively: truncates player name to 255 chars
		uint8_t len = uint8_t(std::min< size_t >(255, player.name.size()));
		connection.send(len);
		connection.send_buffer.insert(connection.send_buffer.end(), player.name.begin(), player.name.begin() + len);

		uint8_t pos_pile_len = player.pos_pile.size();
		connection.send(pos_pile_len);
		for (int i = 0; i < pos_pile_len; i++) {
			connection.send(player.pos_pile.at(i));
		}

		uint8_t neg_pile_len = player.neg_pile.size();
		connection.send(neg_pile_len);
		for (int i = 0; i < neg_pile_len; i++) {
			connection.send(player.neg_pile.at(i));
		}

		uint8_t one_len = player.one.size();
		connection.send(one_len);
		for (int i = 0; i < one_len; i++) {
			connection.send(player.one.at(i));
		}

		uint8_t two_len = player.two.size();
		connection.send(two_len);
		for (int i = 0; i < two_len; i++) {
			connection.send(player.two.at(i));
		}

		uint8_t three_len = player.three.size();
		connection.send(three_len);
		for (int i = 0; i < three_len; i++) {
			connection.send(player.three.at(i));
		}

		uint8_t four_len = player.four.size();
		connection.send(four_len);
		for (int i = 0; i < four_len; i++) {
			connection.send(player.four.at(i));
		}

		connection.send(player.D);
		connection.send(player.H);
		connection.send(player.C);
		connection.send(player.S);
		connection.send(player.score);
		connection.send(player.pos_pos);
		connection.send(player.done);
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player);
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	players.clear();
	uint8_t player_count;
	read(&player_count);
	for (uint8_t i = 0; i < player_count; ++i) {
		players.emplace_back();
		Player &player = players.back();
		read(&player.position);
		read(&player.velocity);
		read(&player.color);
		uint8_t name_len;
		read(&name_len);
		//n.b. would probably be more efficient to directly copy from recv_buffer, but I think this is clearer:
		player.name = "";
		for (uint8_t n = 0; n < name_len; ++n) {
			char c;
			read(&c);
			player.name += c;
		}

		uint8_t pos_pile_len;
		read(&pos_pile_len);
		std::vector<std::tuple<int, int, int, int>> pos_pile;
		for (int j = 0; j < pos_pile_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			pos_pile.push_back(temp);
		}
		player.pos_pile = pos_pile;

		uint8_t neg_pile_len;
		read(&neg_pile_len);
		std::vector<std::tuple<int, int, int, int>> neg_pile;
		for (int j = 0; j < neg_pile_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			neg_pile.push_back(temp);
		}
		player.neg_pile = neg_pile;

		uint8_t one_len;
		read(&one_len);
		std::vector<std::tuple<int, int, int, int>> one;
		for (int j = 0; j < one_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			one.push_back(temp);
		}
		player.one = one;

		uint8_t two_len;
		read(&two_len);
		std::vector<std::tuple<int, int, int, int>> two;
		for (int j = 0; j < two_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			two.push_back(temp);
		}
		player.two = two;

		uint8_t three_len;
		read(&three_len);
		std::vector<std::tuple<int, int, int, int>> three;
		for (int j = 0; j < three_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			three.push_back(temp);
		}
		player.three = three;

		uint8_t four_len;
		read(&four_len);
		std::vector<std::tuple<int, int, int, int>> four;
		for (int j = 0; j < four_len; j++) {
			std::tuple<int, int, int, int> temp;
			read(&temp);
			four.push_back(temp);
		}
		player.four = four;	

		read(&player.D);
		read(&player.H);
		read(&player.C);
		read(&player.S);
		read(&player.score);
		read(&player.pos_pos);
		read(&player.done);
	}

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
