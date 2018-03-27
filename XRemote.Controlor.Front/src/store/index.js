import Vue from 'vue'
import Vuex from 'vuex'
import mutations from './mutations'
import actions from './actions'

Vue.use(Vuex)

const store = new Vuex.Store({
  state: {
    spinShow: false,
    menuData: [],
    menuTop: [],
    menuChild: [],
    btnFlag: [],
    user: {}
  },
  actions: actions,
  mutations: mutations,
  modules: {
  }
})

export default store
