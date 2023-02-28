import React from 'react'
import ReactDOM from 'react-dom/client'
import {AdminPage, ButtonsPage} from './App'

import {
  createBrowserRouter,
  RouterProvider,
} from "react-router-dom";

import './index.css'

const router = createBrowserRouter([
  {
    path: "/admin",
    element: <AdminPage/>,
  },
  {
    path: "/",
    element: <ButtonsPage/>,
  },
]);

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <RouterProvider router={router} />
  </React.StrictMode>,
)
