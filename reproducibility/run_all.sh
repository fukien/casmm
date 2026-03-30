date
start_time=$(date +%s)

cd ../scripts

cd figure1-scripts
bash run_fig1.sh
cd ..

cd figure3-scripts
bash run_fig3.sh
cd ..

cd figure4-scripts
bash run_fig4.sh
cd ..

cd figure5-scripts
bash run_fig5.sh
cd ..

cd figure7-scripts
bash run_fig7.sh
cd ..

cd figure8-scripts
bash run_fig8.sh
cd ..

cd figure9-scripts
bash run_fig9.sh
cd ..

cd figure10-scripts
bash run_fig10.sh
cd ..

cd table1-scripts
bash run_tab1.sh
cd ..

cd table2-scripts
bash run_tab2.sh
cd ..


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date