import React, { useState, useEffect } from 'react';
import DatePicker from 'react-datepicker';
import 'react-datepicker/dist/react-datepicker.css';

function DataPage({ currentTemp, hourlyAvg, dailyAvg }) {
  const [dataType, setDataType] = useState('current'); 
  const [data, setData] = useState([]);
  const [timestamps, setTimestamps] = useState([]);
  const [searchDate, setSearchDate] = useState(null); 
  const [searchResult, setSearchResult] = useState(null); 


  const fetchData = async () => {
    try {
      let endpoint = "";
      switch (dataType) {
        case "current":
          endpoint = "http://localhost:8080/all_temperatures";
          break;
        case "hourly":
          endpoint = "http://localhost:8080/last_hourly_averages";
          break;
        case "daily":
          endpoint = "http://localhost:8080/last_daily_averages"; 
          break;
        default:
          endpoint = "http://localhost:8080/all_temperatures";
      }

      const response = await fetch(endpoint);
      const textData = await response.text(); 
      const lines = textData.split("\n").filter(line => line.trim() !== ""); 

      const parsedData = lines.map(line => {
        const [tempPart, timestampPart] = line.split(", ");
        const temperature = parseFloat(tempPart.split(": ")[1]);
        const timestamp = timestampPart.split(": ")[1];
        return { temperature, timestamp };
      });

      setData(parsedData.map(item => item.temperature));
      setTimestamps(parsedData.map(item => item.timestamp)); 
    } catch (error) {
      console.error("Ошибка при получении данных:", error);
    }
  };


  function formatLocalDateTime(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hour = String(date.getHours()).padStart(2, '0');
    const minute = String(date.getMinutes()).padStart(2, '0');
    const second = String(date.getSeconds()).padStart(2, '0');
    return `${year}-${month}-${day} ${hour}:${minute}:${second}`;
  }
  
  const handleSearch = async () => {
    if (!searchDate) {
      setSearchResult(null);
      return;
    }
  
    const searchTimestamp = formatLocalDateTime(searchDate);
    const encodedTimestamp = encodeURIComponent(searchTimestamp);
  
    try {
      const response = await fetch(`http://localhost:8080/search_temperature?datetime=${encodedTimestamp}`);
      const textData = await response.text();
  
      if (textData === "Not found") {
        setSearchResult(null);
      } else {
        const [tempPart, timestampPart] = textData.split(", ");
        const temperature = parseFloat(tempPart.split(": ")[1]);
        const timestamp = timestampPart.split(": ")[1];
        setSearchResult({ temperature, timestamp });
      }
    } catch (error) {
      console.error("Ошибка при поиске данных:", error);
    }
  };

  useEffect(() => {
    fetchData();
  }, [dataType]); 

  return (
    <div className="data-page">
      <h1>Данные за текущий период</h1>
      <div className="data-filter">
        <button
          className={`filter-button ${dataType === 'current' ? 'active' : ''}`}
          onClick={() => setDataType('current')}
        >
          Текущие данные
        </button>
        <button
          className={`filter-button ${dataType === 'hourly' ? 'active' : ''}`}
          onClick={() => setDataType('hourly')}
        >
          Часовые данные
        </button>
        <button
          className={`filter-button ${dataType === 'daily' ? 'active' : ''}`}
          onClick={() => setDataType('daily')}
        >
          Дневные данные
        </button>
      </div>


      {dataType === 'current' && (
        <div className="search-date">
          <label>Поиск по дате и времени: </label>
          <DatePicker
            selected={searchDate}
            onChange={date => setSearchDate(date)}
            showTimeSelect
            dateFormat="yyyy-MM-dd HH:mm:ss"
            timeFormat="HH:mm:ss"
            placeholderText="Выберите дату и время"
            className="date-picker-input"
          />
          <button onClick={handleSearch}>Найти</button>
        </div>
      )}


      {dataType === 'current' && searchResult && (
        <div className="search-result">
          <h2>Результат поиска:</h2>
          <p>Время: {searchResult.timestamp}</p>
          <p>Температура: {searchResult.temperature} °C</p>
        </div>
      )}


      <div className="data-table">
        <table>
          <thead>
            <tr>
              <th>Время</th>
              <th>Температура (°C)</th>
            </tr>
          </thead>
          <tbody>
            {data.map((value, index) => (
              <tr key={index}>
                <td>{timestamps[index]}</td>
                <td>{value}</td> 
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export default DataPage;