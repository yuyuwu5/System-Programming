########
TODO
########
1. Execution
	make
	bidding_system <host_num> <player_num>
2. Description
	bidding system:
	 *算出此player數下所有的比賽組合
	 *建立pipe後fork host,將child的stdin stdout redirect 到我的pipe,execl host程式
	 *先寫一組資料到每一個pipe裡
	 *在while裡開始送每一組比賽組合，用select來挑可以做事的host
	 *做排序後輸出
	
	host:
	 *建立五個fifo
	 *讀player id,產生random key,fork再execl player程式
	 *開始辦10輪比賽,每次依照規則給每個參賽者錢,把他們有的錢寫進fifo,從fifo讀出他們的出價,判斷誰贏了此次競標，把每個player剩下的錢和贏得次數記錄下來
	 *做排序後輸出
	player:
	 *開fifo來讀寫
	 *紀錄自己讀寫到第幾次來決定此次要不要出價，讀寫十次後關閉
3. Self-Examinatioin
	1. My bidding_system works fine, it ouput correct result.
	2. My bidding_system schedules host effectively by select.
	3. My bidding_system executes host correctly, I only fork host_num
	times.
	4. My host works fine.
	5. My player works fine.
	6. I produce correct result with my bidding_system, host and player.
	7. My Makefile generate bidding_system, host and player well.
