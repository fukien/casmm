date
start_time=$(date +%s)

cd ../scripts

cd figure1-scripts
python plot_fig1.py
cd ..

cd figure3-scripts
python plot_fig3.py
cd ..

cd figure4-scripts
python plot_fig4.py
cd ..

cd figure5-scripts
python plot_adj_nnz.py
python plot_webbase-1M_tps_llc_miss.py
cd ..

cd figure7-scripts
python plot_fig7.py
cd ..

cd figure8-scripts
python plot_fig8.py
cd ..

cd figure9-scripts
python plot_fig9.py
cd ..

cd figure10-scripts
python plot_fig10.py
cd ..


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date