import React, { useEffect, useState } from 'react';
import { BrowserRouter as Router, Routes, Route, Link } from 'react-router-dom';
import { Line } from 'react-chartjs-2';
import CurrentTemperature from './components/CurrentTemperature';
import HourlyAverage from './components/HourlyAverage';
import DailyAverage from './components/DailyAverage';
import DataPage from './components/DataPage'; 
import './App.css';

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

function App() {
  const [currentTemp, setCurrentTemp] = useState(null);
  const [hourlyAvg, setHourlyAvg] = useState(null);
  const [dailyAvg, setDailyAvg] = useState(null);
  const [selectedDataType, setSelectedDataType] = useState('current'); 


  const [currentChartData, setCurrentChartData] = useState({
    labels: [],
    datasets: [
      {
        label: 'Текущая температура',
        data: [],
        borderColor: 'rgba(75, 192, 192, 1)',
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        borderWidth: 2,
      },
    ],
  });

  const [hourlyChartData, setHourlyChartData] = useState({
    labels: [],
    datasets: [
      {
        label: 'Средняя температура за час',
        data: [],
        borderColor: 'rgba(255, 99, 132, 1)',
        backgroundColor: 'rgba(255, 99, 132, 0.2)',
        borderWidth: 2,
      },
    ],
  });

  const [dailyChartData, setDailyChartData] = useState({
    labels: [],
    datasets: [
      {
        label: 'Средняя температура за день',
        data: [],
        borderColor: 'rgba(153, 102, 255, 1)',
        backgroundColor: 'rgba(153, 102, 255, 0.2)',
        borderWidth: 2,
      },
    ],
  });


  const updateCurrentChart = (newData, timestamp) => {
    const timeOnly = timestamp.split(" ")[1]; 
    setCurrentChartData((prevChartData) => {
      const newLabels = [...prevChartData.labels, timeOnly]; 
      const newDatasetData = [...prevChartData.datasets[0].data, newData];
  

      if (newLabels.length > 60) {
        newLabels.shift();
        newDatasetData.shift(); 
      }
  
      return {
        labels: newLabels,
        datasets: [
          {
            ...prevChartData.datasets[0],
            data: newDatasetData,
          },
        ],
      };
    });
  };


  const updateHourlyChart = (averages) => {
    const labels = averages.map(item => item.timestamp); 
    const data = averages.map(item => item.temperature); 

    setHourlyChartData({
      labels: labels.reverse(),
      datasets: [
        {
          label: 'Средняя температура за час',
          data: data.reverse(),
          borderColor: 'rgba(255, 99, 132, 1)',
          backgroundColor: 'rgba(255, 99, 132, 0.2)',
          borderWidth: 2,
        },
      ],
    });
  };


  const updateDailyChart = (averages) => {
    const labels = averages.map(item => item.timestamp); 
    const data = averages.map(item => item.temperature); 

    setDailyChartData({
      labels: labels.reverse(), 
      datasets: [
        {
          label: 'Средняя температура за день',
          data: data.reverse(),
          borderColor: 'rgba(153, 102, 255, 1)',
          backgroundColor: 'rgba(153, 102, 255, 0.2)',
          borderWidth: 2,
        },
      ],
    });
  };


  const fetchCurrentData = async () => {
    try {
      const responseCurrent = await fetch('http://localhost:8080/current_temperature');
      const data = await responseCurrent.text(); 


      const temperatureMatch = data.match(/Temperature: ([\d.]+)/);
      const timestampMatch = data.match(/Timestamp: (.+)/);

      if (temperatureMatch && temperatureMatch[1] && timestampMatch && timestampMatch[1]) {
        const temperatureValue = parseFloat(temperatureMatch[1]);
        const timestamp = timestampMatch[1];

        setCurrentTemp(temperatureValue.toFixed(1)); 
        updateCurrentChart(temperatureValue, timestamp);
      }
    } catch (error) {
      console.error('Ошибка при получении текущей температуры:', error);
    }
  };

  const fetchHourlyData = async () => {
    try {
      const responseHourly = await fetch('http://localhost:8080/last_hourly_averages');
      const data = await responseHourly.text(); 


      const lines = data.split('\n').filter(line => line.trim() !== '');


      const averages = lines.map(line => {
        const temperatureMatch = line.match(/Temperature: ([\d.]+)/);
        const timestampMatch = line.match(/Timestamp: (.+)/);
        if (temperatureMatch && temperatureMatch[1] && timestampMatch && timestampMatch[1]) {
          return {
            temperature: parseFloat(temperatureMatch[1]),
            timestamp: timestampMatch[1],
          };
        }
        return null;
      }).filter(item => item !== null); 

      if (averages.length > 0) {
        setHourlyAvg(averages[0].temperature.toFixed(1)); 
        updateHourlyChart(averages);
      } else {
        setHourlyAvg(null); 
      }
    } catch (error) {
      console.error('Ошибка при получении часовой средней температуры:', error);
    }
  };

  const fetchDailyData = async () => {
    try {
      const responseDaily = await fetch('http://localhost:8080/last_daily_averages');
      const data = await responseDaily.text(); 


      const lines = data.split('\n').filter(line => line.trim() !== '');


      const averages = lines.map(line => {
        const temperatureMatch = line.match(/Temperature: ([\d.]+)/);
        const timestampMatch = line.match(/Timestamp: (.+)/);
        if (temperatureMatch && temperatureMatch[1] && timestampMatch && timestampMatch[1]) {
          return {
            temperature: parseFloat(temperatureMatch[1]),
            timestamp: timestampMatch[1],
          };
        }
        return null;
      }).filter(item => item !== null); 

      if (averages.length > 0) {
        setDailyAvg(averages[0].temperature.toFixed(1));
        updateDailyChart(averages);
      } else {
        setDailyAvg(null); 
      }
    } catch (error) {
      console.error('Ошибка при получении дневной средней температуры:', error);
    }
  };

  useEffect(() => {
    fetchCurrentData();
    fetchHourlyData();
    fetchDailyData();


    const currentInterval = setInterval(fetchCurrentData, 5000); 
    const hourlyInterval = setInterval(fetchHourlyData, 3600000); 
    const dailyInterval = setInterval(fetchDailyData, 86400000); 


    return () => {
      clearInterval(currentInterval);
      clearInterval(hourlyInterval);
      clearInterval(dailyInterval);
    };
  }, []);


  const getChartData = () => {
    switch (selectedDataType) {
      case 'current':
        return currentChartData;
      case 'hourly':
        return hourlyChartData;
      case 'daily':
        return dailyChartData;
      default:
        return currentChartData;
    }
  };

  return (
    <Router>
      <div className="App">
        <div className="top-bar">
          <div className="logo">Погода Прошлого</div>
          <div className="buttons">
            <Link to="/" className="top-bar-button">Главная</Link>
            <Link to="/data" className="top-bar-button">Все данные</Link>
          </div>
        </div>

        <Routes>
          <Route
            path="/"
            element={
              <>
                <div className="dashboard">
                  <div className={`card ${selectedDataType === 'current' ? 'selected' : ''}`} onClick={() => setSelectedDataType('current')}>
                    <CurrentTemperature temperature={currentTemp} />
                  </div>
                  <div className={`card ${selectedDataType === 'hourly' ? 'selected' : ''}`} onClick={() => setSelectedDataType('hourly')}>
                    <HourlyAverage average={hourlyAvg} />
                  </div>
                  <div className={`card ${selectedDataType === 'daily' ? 'selected' : ''}`} onClick={() => setSelectedDataType('daily')}>
                    <DailyAverage average={dailyAvg} />
                  </div>
                </div>
                <div className="chart-container">
                  <h2>
                    {selectedDataType === 'current' && 'Текущая температура с интервалом 5 секунд'}
                    {selectedDataType === 'hourly' && 'Средняя температура за последние 5 часов'}
                    {selectedDataType === 'daily' && 'Средняя температура за последние 5 дней'}
                  </h2>
                  <Line data={getChartData()} />
                </div>
              </>
            }
          />
          <Route
            path="/data"
            element={
              <DataPage
                currentTemp={currentTemp}
                hourlyAvg={hourlyAvg}
                dailyAvg={dailyAvg}
              />
            }
          />
        </Routes>
      </div>
    </Router>
  );
}

export default App;