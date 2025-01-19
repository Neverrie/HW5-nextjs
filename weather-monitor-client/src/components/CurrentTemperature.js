import React from 'react';

function CurrentTemperature({ temperature }) {
  return (
    <div>
      <h2>Текущая температура</h2>
      <p>{temperature ? `${temperature} °C` : 'Загрузка...'}</p>
    </div>
  );
}

export default CurrentTemperature;