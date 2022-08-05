----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 07/28/2022 09:24:39 PM
-- Design Name: 
-- Module Name: tuner_core_tb - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity tuner_core_tb is
--  Port ( );
end tuner_core_tb;

architecture Behavioral of tuner_core_tb is
  component tuner_core is
        Port (
            clk125 : in std_logic;
            reset : in std_logic;
            tuner_pinc : in std_logic_vector(31 downto 0);
            fake_adc_pinc : in std_logic_vector(31 downto 0);
            m_axis_data_tdata : out std_logic_vector(31 downto 0);
            m_axis_data_tvalid : out std_logic
        );
    end component tuner_core;
    constant TbPeriod : time := 8 ns;
    signal TbClock : std_logic := '0';
    
    signal clk : std_logic;
    signal tuner_pinc : std_logic_vector(31 downto 0);
    signal adc_pinc : std_logic_vector(31 downto 0);
    signal tuner_data : std_logic_vector(31 downto 0);
    signal tuner_valid : std_logic;
    signal reset : std_logic := '0';
begin

    TbClock <= not TbClock after TbPeriod/2;
    clk <= TbClock;
    
    tuner_pinc <= std_logic_vector(to_signed(300000, tuner_pinc'length));
    adc_pinc <= std_logic_vector(to_signed(315000, adc_pinc'length));
    radio_tuner : component tuner_core
    port map (
        clk125 => clk,
        reset => reset,
        tuner_pinc => tuner_pinc,
        fake_adc_pinc => adc_pinc,
        m_axis_data_tdata => tuner_data,
        m_axis_data_tvalid => tuner_valid 
    );


end Behavioral;
