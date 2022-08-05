----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 07/28/2022 08:17:08 PM
-- Design Name: 
-- Module Name: tuner_core - Behavioral
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

entity tuner_core is
    Port (
        clk125 : in std_logic;
        reset : in std_logic;
        tuner_pinc : in std_logic_vector(31 downto 0);
        fake_adc_pinc : in std_logic_vector(31 downto 0);
        m_axis_data_tdata : out std_logic_vector(31 downto 0);
        m_axis_data_tvalid : out std_logic
    );
end tuner_core;

architecture Behavioral of tuner_core is
    COMPONENT dds_compiler_0
        PORT (
            aclk : IN STD_LOGIC;
            aresetn : IN STD_LOGIC;
            s_axis_config_tvalid : IN STD_LOGIC;
            s_axis_config_tdata : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_data_tvalid : OUT STD_LOGIC;
            m_axis_data_tdata : OUT STD_LOGIC_VECTOR(15 DOWNTO 0)
        );
    END COMPONENT;
    COMPONENT dds_compiler_1
        PORT (
            aclk : IN STD_LOGIC;
            aresetn : IN STD_LOGIC;
            s_axis_config_tvalid : IN STD_LOGIC;
            s_axis_config_tdata : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_data_tvalid : OUT STD_LOGIC;
            m_axis_data_tdata : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
        );
    END COMPONENT;
    COMPONENT fir_compiler_0
        PORT (
            aclk : IN STD_LOGIC;
            s_axis_data_tvalid : IN STD_LOGIC;
            s_axis_data_tready : OUT STD_LOGIC;
            s_axis_data_tdata : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_data_tvalid : OUT STD_LOGIC;
            m_axis_data_tdata : OUT STD_LOGIC_VECTOR(47 DOWNTO 0)
        );
    END COMPONENT;
    COMPONENT fir_compiler_1
        PORT (
            aclk : IN STD_LOGIC;
            s_axis_data_tvalid : IN STD_LOGIC;
            s_axis_data_tready : OUT STD_LOGIC;
            s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
            m_axis_data_tvalid : OUT STD_LOGIC;
            m_axis_data_tdata : OUT STD_LOGIC_VECTOR(47 DOWNTO 0)
        );
    END COMPONENT;
    signal fake_adc_tdata : std_logic_vector(15 downto 0);
    signal tuner_tdata : std_logic_vector(31 downto 0);
    signal mult_tdata_i : std_logic_vector(31 downto 0);
    signal mult_tdata_q : std_logic_vector(31 downto 0);
    signal mult_tdata : std_logic_vector(31 downto 0);

    signal filt_1_tdata : std_logic_vector(47 downto 0);
    signal filt_1_tvalid : std_logic;

    signal filt_2_tdata : std_logic_vector(47 downto 0);

begin
    fake_adc : dds_compiler_0
        PORT MAP (
            aclk => clk125,
            aresetn => (not reset),
            s_axis_config_tvalid => '1',
            s_axis_config_tdata => fake_adc_pinc,
            m_axis_data_tvalid => open,
            m_axis_data_tdata => fake_adc_tdata
        );
    tuner_dds : dds_compiler_1
        PORT MAP (
            aclk => clk125,
            aresetn => (not reset),
            s_axis_config_tvalid => '1',
            s_axis_config_tdata => tuner_pinc,
            m_axis_data_tvalid => open,
            m_axis_data_tdata => tuner_tdata
        );
    mult_tdata_i <= std_logic_vector(signed(tuner_tdata(31 downto 16)) * signed(fake_adc_tdata));
    mult_tdata_q <= std_logic_vector(signed(tuner_tdata(15 downto 0)) * signed(fake_adc_tdata));
    mult_tdata(31 downto 16) <= mult_tdata_i(29 downto 14);
    mult_tdata(15 downto 0) <= mult_tdata_q(29 downto 14);
    lowpass_dec_filter_1 : fir_compiler_0
        PORT MAP (
            aclk => clk125,
            s_axis_data_tvalid => '1',
            s_axis_data_tready => open,
            s_axis_data_tdata => mult_tdata,
            m_axis_data_tvalid => filt_1_tvalid,
            m_axis_data_tdata => filt_1_tdata
        );
    lowpass_dec_filter_2 : fir_compiler_1
        PORT MAP (
            aclk => clk125,
            s_axis_data_tvalid => filt_1_tvalid,
            s_axis_data_tready => open,
            s_axis_data_tdata => filt_1_tdata,
            m_axis_data_tvalid => m_axis_data_tvalid,
            m_axis_data_tdata => filt_2_tdata
        );
    m_axis_data_tdata(31 downto 16) <= filt_2_tdata(39 downto 24);
    m_axis_data_tdata(15 downto 0) <= filt_2_tdata(15 downto 0);
end Behavioral;
