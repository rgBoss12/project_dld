--
-- Copyright (C) 2009-2012 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_misc.all;

architecture rtl of swled is
	-- Flags for display on the 7-seg decimal points
	signal flags                   : std_logic_vector(3 downto 0);

	-- Registers implementing the channels
	signal checksum, checksum_next 	: std_logic_vector(15 downto 0) 	:= (others => '0');
	signal reg0, reg0_next         	: std_logic_vector(7 downto 0)  	:= (others => '0');
	signal state				   	: std_logic_vector(31 downto 0)  := (others => '0');
	signal state_next			   	: STD_LOGIC_VECTOR(31 downto 0) 	:= (others => '0');
	signal counter                 	: std_logic_vector(2 downto 0) 	:= ("100");
	signal counter_next			   	: std_logic_vector(2 downto 0)  	:= (others => '0');
	signal counter_show            	: std_logic_vector(2 downto 0) 	:= (others => '0');
	signal counter_send            	: std_logic_vector(2 downto 0) 	:= (others => '0');
	signal timer                   	: integer range 0 to 12288000000;
	signal key					   	: std_logic_vector(31 downto 0) 	:= ("00000000000000000000000000000001");
	signal plain_text			   	: std_logic_vector(31 downto 0) 	:= (others => '0');
	signal done1				   	: STD_LOGIC := '0';
	signal done3				   	: STD_LOGIC := '0';
	signal start_dec				: std_logic := '0';
	signal reset_dec				: std_logic := '1';
	signal X 						: STD_LOGIC_VECTOR(3 downto 0) := "0010";
	signal Y						: STD_LOGIC_VECTOR(3 downto 0) := "0010";
	signal cypher_text				: STD_LOGIC_VECTOR(31 downto 0);
	signal f2hData_out_next         : STD_LOGIC_VECTOR(7 downto 0);
	signal f2hData_out_signal       : STD_LOGIC_VECTOR(7 downto 0);
	signal coordinates_done     : STD_LOGIC :='0';
	signal ack1     			: STD_LOGIC :='0';
	signal final_states         : STD_LOGIC_VECTOR(63 downto 0) :=(others => '0');
	signal first_32_bits_done,second_32_bits_done,show_time  : STD_LOGIC := '0';
	signal recieving            : STD_LOGIC := '0';
	signal ack_enc_done 				: std_logic;
	signal cypher_text_ack			: std_logic_vector(31 downto 0);
	signal dir_enc				: std_logic_vector(2 downto 0);
	signal counter_imdt			: std_logic_vector(1 downto 0)  := (others => '0');

	component decrypter
		port(clock : in  STD_LOGIC;
			  K : in  STD_LOGIC_VECTOR (31 downto 0);
			  C : in  STD_LOGIC_VECTOR (31 downto 0);
			  P : out  STD_LOGIC_VECTOR (31 downto 0);
			  done : out STD_LOGIC;
			  reset : in  STD_LOGIC;
			  enable : in  STD_LOGIC);
	end component;

	component encrypter
		port(clock : in  STD_LOGIC;
			  K : in  STD_LOGIC_VECTOR (31 downto 0);
			  P : in  STD_LOGIC_VECTOR (31 downto 0);
			  C : out  STD_LOGIC_VECTOR (31 downto 0);
			  done : out STD_LOGIC;
			  reset : in  STD_LOGIC;
			  enable : in  STD_LOGIC);
	end component;

begin                                                                     --BEGIN_SNIPPET(registers)
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				reg0 <= (others => '0');
				checksum <= (others => '0');
				counter<="100";
			else
				if (recieving='1') then

					counter <= counter_next;
					state <= state_next;
				 	if (to_integer(unsigned(counter))=0) then
				 		if(done1='1') then
			 				reset_dec <= '1';
			 				start_dec <= '0';
			 				counter <= "100";
				 			if(coordinates_done='0') then

				 				if(ack1='0') then
					 				if(plain_text ="000000000000000000000000"&X&Y) then             --check if coordinates send by host are correct
						 				ack1<='1';
						 				f2hValid_out <= '1';
						 				recieving<='0';
					 					h2fReady_out <= '0';
										timer <= 0;
									else
										f2hValid_out <= '0';
										h2fReady_out<= '1';
										timer <= timer + 1;
				 					end if;
				 				elsif (ack1='1') then
				 					if(plain_text = "00000000000000000000000000000001") then         --check if special encrypted element(ack2) from host as expcted or not
					 					coordinates_done<='1';
					 					ack1<='0';
					 					recieving<='1';
					 					f2hValid_out <= '0';
					 					h2fReady_out <= '1';
										timer <= 0;
									else
										f2hValid_out <= '0';
										h2fReady_out<= '1';
										timer <= timer + 1;
				 					end if;
				 				end if;
				 			else

				 				if(first_32_bits_done='0') then
				 					final_states(63 downto 32) <= plain_text;
				 					first_32_bits_done<='1';
				 					ack1<='1';
				 					recieving<='0';
				 					f2hValid_out <= '1';
			 						h2fReady_out <= '0';
				 				elsif(first_32_bits_done='1' and second_32_bits_done<='0')then
				 					final_states(31 downto 0) <= plain_text;
				 					second_32_bits_done<='1';
				 					ack1<='1';
				 					recieving<='0';
				 					f2hValid_out <= '1';
			 						h2fReady_out <= '0';
				 				else
				 					if(plain_text = "00000000000000000000000000000001") then--check if special encrypted element(ack2) from host as expcted or not
				 						first_32_bits_done<='0';
				 						second_32_bits_done<='0';
				 						show_time<='1';
				 						--coordinates_done<='0';
				 						recieving<='0';
				 						--f2hValid_out <= '0';
				 						h2fReady_out <= '0';
										timer <= 0;
									else
										f2hValid_out <= '0';
										h2fReady_out<= '1';
										timer <= timer + 1;
				 					end if;
				 				end if;
				 			end if;
				 		else
			 				reset_dec<='0';
			 				start_dec<='1';
			 				recieving<='1';
							f2hValid_out <= '0';
							h2fReady_out<= '0';
							timer <= timer + 1;

			 			end if;
				 	else
							if (coordinates_done='0' or (coordinates_done='1' and first_32_bits_done='1' and second_32_bits_done='1')) then
								if (timer >= 256 * 47999999) then
									timer <= 0;
									recieving <= '0';
									show_time <= '0';
									h2fReady_out <= '0';
									f2hvalid_out <= '0';
									coordinates_done <= '0';
									ack1 <= '0';
									counter <= "100";
									first_32_bits_done <= '0';
									second_32_bits_done <= '0';
								else
									recieving <= '1';
									f2hValid_out <= '0';
									h2fReady_out <= '1';
									timer <= timer + 1;
								end if;
							else
								recieving <= '1';
								f2hValid_out <= '0';
								h2fReady_out <= '1';
							end if;
					end if;
				elsif (show_time='1') then
				 		if(to_integer(unsigned(counter_show))<7) then
						 	reg0 <=reg0_next;
						 	if(timer=47999999)then
						 		counter_show<=counter_show+1;
								counter_imdt <= "00";
						 		timer<=0;
						 	else
								if (timer=47999999) then
						 			counter_imdt <= "01";
						 		elsif (timer=2*47999999) then
						 			counter_imdt <= "10";
						 		end if ;
					 			timer<=timer+1;
						 	end if;
				 		elsif (to_integer(unsigned(counter_show))=7) then
						 		reg0<=reg0_next;
						 		if (timer=(47999999*9)) then
						 			reg0<="00000000";
							 		---************
										timer<=0;
										counter_imdt <= "00";
								 		show_time<='0';
										--X<="010";
										--Y<="010";
										counter_show<="000";
										--counter_send<="000";
										--counter_next<="000";
										state<= (others => '0');
										--state_next<=(others => '0');
										timer<=0;
										reset_dec<='1';
										start_dec <= '0';
										show_time<='0';
										f2hValid_out <= '0';
										recieving<='0';
										h2fReady_out <= '0';
										first_32_bits_done<='0';
										second_32_bits_done<='0';
										--f2hData_out_next<=(others=> '0');
										final_states<=(others=> '0');
										--show_time<='0';
										coordinates_done<='0';
										ack1<='0';
										--done1<='0';
										--done3<='0';
										start_dec<='0';
										reset_dec<='1';
										--key<=(others=> '0');
										--plain_text<=(others=> '0');

							 		---************
						 		else
									if (timer=47999999) then
							 			counter_imdt <= "01";
							 		elsif (timer=2*47999999) then
							 			counter_imdt <= "10";
							 		end if ;
						 			timer<=timer+1;
						 		end if ;
						end if;
				else
					--if(f2hReady_in='1') then
						f2hData_out_signal <= f2hData_out_next;
						if(counter_send="100") then
							counter_send<="000";
							f2hValid_out<='0';
							recieving<='1';
							h2fReady_out<='1';
						elsif(f2hReady_in = '1') then
							f2hValid_out<='1';
							counter_send<=counter_send + "001";
						end if;
					--end if;
				end if;
			end if;
		end if;
	end process;

	-- Drive register inputs for each channel when the host is writing
	state_next((8*(to_integer(unsigned(counter))-1)+7) downto (8*(to_integer(unsigned(counter))-1))) <=
		h2fData_in when chanAddr_in = "0000001" and h2fValid_in = '1'
		else state((8*(to_integer(unsigned(counter))-1)+7) downto (8*(to_integer(unsigned(counter))-1)));

	counter_next<=
		counter-"001" when chanAddr_in="0000001" and h2fValid_in='1'
		--std_logic_vector(unsigned(counter)+unsigned('001')) when chanAddr_in = "0000000" and h2fValid_in = '1'
		else counter;
--------------------------------------------------------------------------------------------------------------------------------------------------
reg0_next <=
	(final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00001")
		when (final_states(8*to_integer(unsigned(counter_show))+6)='0' or final_states(8*to_integer(unsigned(counter_show))+7)='0' or sw_in(to_integer(unsigned(counter_show)))='0')
	else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00100")
		when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and sw_in(to_integer(unsigned(dir_enc))) = '1' and sw_in(to_integer((unsigned(dir_enc)) + 4)rem 8) = '0'
	else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00001")
		when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and sw_in(to_integer(unsigned(dir_enc))) = '1' and sw_in(to_integer((unsigned(dir_enc)) + 4)rem 8) = '1' and to_integer(unsigned(dir_enc)) < 4
	else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00100")
		when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and sw_in(to_integer(unsigned(dir_enc))) = '1' and sw_in(to_integer((unsigned(dir_enc)) + 4)rem 8) = '1' and to_integer(unsigned(dir_enc)) > 3 and counter_imdt="00"
	else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00010")
		when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and sw_in(to_integer(unsigned(dir_enc))) = '1' and sw_in(to_integer((unsigned(dir_enc)) + 4)rem 8) = '1' and to_integer(unsigned(dir_enc)) > 3 and counter_imdt="01"
	else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00001")
		when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and sw_in(to_integer(unsigned(dir_enc))) = '1' and sw_in(to_integer((unsigned(dir_enc)) + 4)rem 8) = '1' and to_integer(unsigned(dir_enc)) > 3 and counter_imdt="10"
	--else (final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3))&"00010")
	--	when final_states(8*to_integer(unsigned(counter_show))+6)='1' and final_states(8*to_integer(unsigned(counter_show))+7)='1' and to_integer(unsigned(final_states((8*to_integer(unsigned(counter_show))+2) downto (8*to_integer(unsigned(counter_show))))))<=1
	else reg0;

dir_enc <= final_states((8*to_integer(unsigned(counter_show))+5) downto (8*to_integer(unsigned(counter_show))+3));
---------------------------------------------------------------------------------------------------------------------------------------------------
	---------**************
	f2hData_out_next <=
		cypher_text((8*(3-to_integer(unsigned(counter_send)))+7) downto (8*(3-to_integer(unsigned(counter_send)))+0))
			when coordinates_done='0' and ack1='0' and done3='1'
		else cypher_text_ack((8*(3-to_integer(unsigned(counter_send)))+7) downto (8*(3-to_integer(unsigned(counter_send)))+0))
			when ack1='1'
		else f2hData_out_signal;

	f2hData_out<=f2hData_out_signal;
		when chanAddr_in ="00000000" and f2hReady_in='1';
	---------**************


	--	std_logic_vector(unsigned(checksum) + unsigned(h2fData_in))
	--		when chanAddr_in = "0000000" and h2fValid_in = '1'
	--	else h2fData_in & checksum(7 downto 0)
	--		when chanAddr_in = "0000001" and h2fValid_in = '1'
	--	else checksum(15 downto 8) & h2fData_in
	--		when chanAddr_in = "0000010" and h2fValid_in = '1'
	--	else checksum;

	-- Select values to return for each channel when the host is reading
	decrypt1: decrypter
		port map(clock=>clk_in,
				K=> key,
				C=>state,
				P=>plain_text,
				done=> done1,
				reset=>reset_dec,
				enable=>start_dec);

	--decrypt2: decrypter
	--	port map(clock=>clk_in,
	--			K=> key,
	--			C=>state(63 downto 32),
	--			P=>plain_text(63 downto 32),
	--			done=> done2,
	--			reset=>reset_dec,
	--			enable=>start_dec);
	encrypt: encrypter
		port map(clock=>clk_in,
				K=>key,
				P=>"000000000000000000000000"&X&Y,
				C=>cypher_text,
				done=>done3,
				reset=>'0',
				enable=>'1');

	encrypt1: encrypter
		port map(clock=>clk_in,
				K=>key,
				P=>"00100001001000010010000100100001",
				C=>cypher_text_ack,
				done=>ack_enc_done,
				reset=>'0',
				enable=>'1');

	--with chanAddr_in select f2hData_out <=
	--	--sw_in                 when "0000000",
	--	plain_text(15 downto 8) when "0000001",
	--	--checksum(7 downto 0)  when "0000010",
	--	x"00" when others;

	-- Assert that there's always data for reading, and always room for writing
	--f2hValid_out <= '1';
	--h2fReady_out <= '1';                                                    --END_SNIPPET(registers)

	-- LEDs and 7-seg display
	led_out <= reg0;
	flags <= "00" & f2hReady_in & reset_in;
	seven_seg : entity work.seven_seg
		port map(
			clk_in     => clk_in,
			data_in    => checksum,
			dots_in    => flags,
			segs_out   => sseg_out,
			anodes_out => anode_out
		);
end architecture;
